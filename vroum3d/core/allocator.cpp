#include "allocator.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

void Allocator::init(VkDevice dev, VkPhysicalDevice pdev)
{
  m_device = dev;
  m_pdev = pdev;

  
}

Allocator::block_ptr_t Allocator::allocate_vk_block(std::uint32_t idx)
{
  VkMemoryAllocateInfo mai{
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    nullptr,
    VkDeviceSize(c_largest_block_size),
    idx
  };

  VkDeviceMemory hdl;

  vk_check(vkAllocateMemory(m_device, &mai, nullptr, &hdl));

  block_ptr_t new_block = m_bag.allocate();

  new_block->size = c_largest_block_size;
  new_block->offset = 0;
  new_block->mem_handle = hdl;
  new_block->mem_idx = idx;

  m_mem_handles.insert(hdl);

  return new_block;
}

void Allocator::destroy()
{
  for(auto& elem : m_mem_handles)
  {
    vkFreeMemory(m_device, elem, nullptr);
  }
  m_mem_handles.clear();
}

void Allocator::insert_block(std::uint32_t mem_idx, block_ptr_t block)
{
  auto& bin = m_prim_bins[mem_idx];

  auto addr = bin.addr(block->size);
  block_ptr_t& oldblock = bin.head_ptr_at(addr);

  oldblock->list_prev = block;

  block->list_next = oldblock;
  block->free = true;
  oldblock = block;

  bin.set_free_flag(addr);
}

Allocator::block_ptr_t Allocator::get_block(std::uint32_t mem_idx, VkDeviceSize size)
{
  auto& bin = m_prim_bins[mem_idx];
  auto [idx_prim, idx_sec] = bin.addr(size);

  std::uint32_t flag_prim = 1ull << idx_prim;

  if(!(bin.free & flag_prim)) // No right secondary bin
  {
    idx_sec = 0; // Reset search for secondary block.

    do {
      idx_prim++;
      flag_prim <<= 1;
    }while (!(flag_prim & bin.free) && idx_prim < c_prim_bin_size);

    if(idx_prim == c_prim_bin_size) // No more blocks !
    {
      return allocate_vk_block(mem_idx);
    }
  }

  auto& secbin = bin.sec_bins[idx_prim];
  
  for(std::size_t flag_sec = 1ull << idx_sec; !(flag_sec & secbin.free);)
  {
    idx_sec++;
    flag_sec <<= 1;
  }

  // Found the list head!
  auto& head = secbin.list_heads[idx_sec];
  block_ptr_t block = head;

  block->list_next->list_prev = nullptr;
  head = head->list_next;
  block->free = false;
  return block;
}

Allocator::block_ptr_t Allocator::allocate_inner(std::uint32_t mem_idx, VkDeviceSize size)
{
  auto block = get_block(mem_idx, size);

  if(block->size != size) // Larger block. Split
  {
    auto residual = split_block(size, block);
    insert_block(mem_idx, residual);
  }

  return block;
}

Allocator::block_ptr_t Allocator::split_block(VkDeviceSize size, block_ptr_t block)
{
  block_ptr_t remain_block = m_bag.allocate();

  remain_block->size = block->size - size;
  remain_block->mem_handle = block->mem_handle;
  remain_block->offset = block->offset + size;
  remain_block->mem_idx = block->mem_idx;

  block->size = size;

  // Edit the 4 pointers in the linked list
  remain_block->phys_next = block->phys_next;
  if(block->phys_next)
    remain_block->phys_next->phys_prev = remain_block;

  remain_block->phys_prev = block;
  block->phys_next = remain_block;

  return remain_block;
}

void Allocator::merge(block_ptr_t b)
{
  auto remove_free_list = [&](block_ptr_t removed)
  {
    // Merge with next; first remove next from its free list
    if(!removed->list_prev) // Find it ourselves
    {
      auto& prim_bin = m_prim_bins[removed->mem_idx];
      auto& list_head = prim_bin.head_ptr_at(prim_bin.addr(removed->size));

      list_head = list_head->list_next;
      list_head->list_prev = nullptr;
    }
    else {
      // Edit 2 pointers for linked list remove
      if(removed->list_next)
        removed->list_next->list_prev = removed->list_prev;

      removed->list_prev->list_next = removed->list_next;
    }
  };

  if(auto next_block = b->phys_next; next_block && next_block->free)
  {
    remove_free_list(next_block);

    b->size += next_block->size;
    m_bag.release(next_block);
  }

  if(auto prev_block = b->phys_prev; prev_block && prev_block->free)
  {
    remove_free_list(prev_block);

    // Similar to before
    b->size += prev_block->size;
    b->offset = prev_block->offset;
    m_bag.release(prev_block);
  }
}

void Allocator::free(block_ptr_t b)
{
  merge(b);
  insert_block(b->mem_idx, b);
}

}
