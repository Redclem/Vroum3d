#include "allocator.h"
#include <vulkan/vulkan_core.h>

#include <cassert>

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

  if(oldblock)
    oldblock->list_prev = block;
  else // Set free flag
    bin.set_free_flag(addr);

  block->list_next = oldblock;
  block->free = true;
  oldblock = block;

}

Allocator::block_ptr_t Allocator::get_block(std::uint32_t mem_idx, VkDeviceSize size)
{
  // TODO : implement and use first set bit functions here.

  auto& bin = m_prim_bins[mem_idx];
  auto [idx_prim, idx_sec] = bin.addr(size);

  // Increment bin
  if(idx_sec == c_sec_bin_size - 1)
  {
    idx_prim++;

    if(idx_prim == c_prim_bin_size)
      return allocate_vk_block(mem_idx);

    idx_sec = 0;

  }
  else {
    idx_sec++;
  }

  // Find non empty bin
  std::uint32_t flag_prim = 1ull << idx_prim;

  if(!(bin.free & flag_prim) // No right secondary bin
    || !(bin.sec_bins[idx_prim].free >> idx_sec)) // Or no right secondary bin
  {
    idx_sec = 0; // Reset search for secondary block.

    
    //auto bit = fsb((bin.free >> idx_prim) >> 1);
    //auto idx_prim_alt = (bit == c_fsb_no_bit ? c_prim_bin_size : idx_prim + bit + 1);
    //(void)(idx_prim_alt);
    /*assert(idx_prim == idx_prim_alt);*/

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
  assert(secbin.free >> idx_sec);
  
  for(std::size_t flag_sec = 1ull << idx_sec; !(flag_sec & secbin.free);)
  {
    idx_sec++;
    flag_sec <<= 1;
  }

  // Found the list head!
  auto& head = secbin.list_heads[idx_sec];
  block_ptr_t block = head;

  if(auto ln = head->list_next; ln)
  {
    ln->list_prev = nullptr;
    head = ln;
  }
  else
  {
    bin.unset_free_flag({idx_prim, idx_sec});
    head = nullptr;
  }

  // Manage free flags

  block->free = false;
  assert(block->size >= size);
  return block;
}

Allocator::block_ptr_t Allocator::allocate_inner(std::uint32_t mem_idx, VkDeviceSize size, VkDeviceSize alignment)
{
  auto block = get_block(mem_idx, size + alignment - 1);

  // Align
  if(auto aligndef = block->offset % alignment; aligndef)
  {
    auto added_bytes = alignment - aligndef;
    block->offset += added_bytes;
    // There must be a physical previous block
    
    auto pb = block->phys_prev;
    if(pb->free)
    {
      auto init_addr = PrimaryBin::addr(pb->size);

      block->phys_prev->size += added_bytes;

      auto new_addr = PrimaryBin::addr(pb->size);

      if(new_addr != init_addr)

    }
  }

  if(block->size != size) // Larger block. Split
  {
    auto residual = split_block(size, block);
    insert_block(mem_idx, residual);
  }

  return block;
}

Allocator::block_ptr_t Allocator::split_block(VkDeviceSize size, block_ptr_t block)
{
  assert(block->size >= size);
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

  assert(block->size + block->offset <= c_largest_block_size);
  assert(remain_block->size + remain_block->offset <= c_largest_block_size);

  assert(block->size + remain_block->size <= c_largest_block_size);

  return remain_block;
}

void Allocator::merge(block_ptr_t b)
{
  auto remove_free_list = [&](block_ptr_t removed)
  {
    // Remove from the free list; either it has a previous block in free list or not
    if(!removed->list_prev) // Find it ourselves
    {
      auto& prim_bin = m_prim_bins[removed->mem_idx];
      auto addr = prim_bin.addr(removed->size);
      auto& list_head = prim_bin.head_ptr_at(addr); // List head is this block
      assert(list_head == removed);

      if(auto lhn = list_head->list_next; lhn)
      {
          lhn->list_prev = nullptr;
          list_head = lhn;
      }
      else {
        list_head = nullptr;
        prim_bin.unset_free_flag(addr);
      }
    }
    else {
      // Edit 2 pointers for linked list remove; do not care for ptr of remove block
      // Previous block exists and list is not empty
      if(removed->list_next)
        removed->list_next->list_prev = removed->list_prev;

      removed->list_prev->list_next = removed->list_next;
    }
  };

  if(auto next_block = b->phys_next; next_block && next_block->free)
  {
    assert(next_block->mem_handle == b->mem_handle);
    assert(b->size + next_block->size <= c_largest_block_size);

    remove_free_list(next_block);

    // Update physical list
    if(next_block->phys_next)
      next_block->phys_next->phys_prev = b;

    b->phys_next = next_block->phys_next;

    // Adjust block size
    b->size += next_block->size;

    m_bag.release(next_block);
  }

  if(auto prev_block = b->phys_prev; prev_block && prev_block->free)
  {
    assert(prev_block->mem_handle == b->mem_handle);
    assert(b->size + prev_block->size <= c_largest_block_size);

    remove_free_list(prev_block);

    // Update phyiscal list
    if(prev_block->phys_prev)
      prev_block->phys_prev->phys_next = b;
    
    b->phys_prev = prev_block->phys_prev;

    // Adjust block size and offset
    b->size += prev_block->size;
    b->offset = prev_block->offset;

    m_bag.release(prev_block);
  }

  if(b->phys_next)
    assert(b->size + b->phys_next->size <= c_largest_block_size);

  if(b->phys_prev)
    assert(b->size + b->phys_prev->size <= c_largest_block_size);

}

void Allocator::free(block_ptr_t b)
{
  merge(b);
  insert_block(b->mem_idx, b);
}

}
