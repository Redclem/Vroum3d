#include "allocator.h"
#include "../testing.h"

#include <filesystem>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include <cassert>

namespace Vroum3d::Core
{

void Allocator::init(VkDevice dev, VkPhysicalDevice pdev)
{
  m_device = dev;

  vkGetPhysicalDeviceMemoryProperties(pdev, &m_mem_props);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(pdev, &props);

	m_nonCoherentAtomSize = props.limits.nonCoherentAtomSize;
}

Allocator::block_ptr_t Allocator::allocate_device_block(std::uint32_t idx)
{
  union mh_t
  {
    VkDeviceMemory dm;
		device_block_ptr_t mh = nullptr;

    operator VkDeviceMemory() const {return dm;}
    operator std::uint64_t() const {return mh;}
  } mh;

	auto device_block = m_device_blocks.allocate();

  if constexpr(!c_dry_allocation)
  {
    VkMemoryAllocateInfo mai{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      nullptr,
      VkDeviceSize(c_largest_block_size),
      idx
    };

    vk_check(vkAllocateMemory(m_device, &mai, nullptr, &mh.dm));
  }
  else
    mh.mh = device_block;
  
	device_block->mem_handle = mh;
	device_block->idx = idx;
	device_block->visible = m_mem_props.memoryTypes[idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	device_block->coherent = m_mem_props.memoryTypes[idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	if(m_first_device_block)
		m_first_device_block->prev = device_block;
	
	device_block->next = m_first_device_block;
	m_first_device_block = device_block;

  block_ptr_t new_block = m_bag.allocate();

  new_block->size = c_largest_block_size;
  new_block->offset = 0;
  new_block->mem_handle = mh;
  new_block->mem_idx = idx;
	new_block->device_memory_block = device_block;
 
  return new_block;
}

void Allocator::destroy()
{
  if constexpr(!c_dry_allocation)
  {
		if constexpr(c_testing)
		{
			if(m_first_device_block)
				throw std::runtime_error("Allocator has allocated device blocks at destruction");
		}

    for(device_block_ptr_t b(m_first_device_block); b;)
    {
      vkFreeMemory(m_device, b->mem_handle, nullptr);
			auto tmp = b->next;
			m_device_blocks.release(b);
			b = tmp;
    }
  }
}

void Allocator::insert_block(std::uint32_t mem_idx, block_ptr_t block, PrimaryBin::addr_t addr)
{
  auto& bin = m_prim_bins[mem_idx];
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
      return allocate_device_block(mem_idx);

    idx_sec = 0;

  }
  else {
    idx_sec++;
  }

  // Find non empty bin

  if(!(bin.free & (1ull << idx_prim)) // No right sec bin
    || !(bin.sec_bins[idx_prim].free >> idx_sec)) // Or no right list in sec bin 
  {
    idx_sec = 0; // Reset search for secondary block.

    auto free_shift = (bin.free >> idx_prim) & ~1ull;
    
    if(free_shift == 0) // No more blocks !
    {
      return allocate_device_block(mem_idx);
    }
    
    idx_prim = idx_prim + fsb(free_shift);
  }

  auto& secbin = bin.sec_bins[idx_prim];
  assert(secbin.free >> idx_sec);

  idx_sec += fsb(secbin.free >> idx_sec);

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
  if(size > c_largest_block_size)
    throw std::runtime_error("Allocation of GPU memory block larger than maximum block size");

  auto block = get_block(mem_idx, size + alignment - 1);

  // Align
  if(auto aligndef = block->offset % alignment; aligndef)
  {
    auto added_bytes = alignment - aligndef;
    // There must be a physical previous block
    
    auto pb = block->phys_prev;
    if(pb->free) // Enlarge previous free block
    {
      auto init_addr = PrimaryBin::addr(pb->size);
      auto new_size = pb->size + added_bytes;

      auto new_addr = PrimaryBin::addr(pb->size);

      if(new_addr != init_addr)
      {
        remove_free_list(pb);
        pb->size = new_size;
        insert_block(mem_idx, pb, new_addr);
      }
      else {
        pb->size = new_size;
      }
    }
    else { // Create new free block
      auto nb = m_bag.allocate();
      nb->size = added_bytes;
      nb->offset = block->offset;
      nb->mem_handle = block->mem_handle;
			nb->device_memory_block = block->device_memory_block;
      nb->mem_idx = block->mem_idx;

      // 4 pts to change in phys list 
      nb->phys_next = block;
      nb->phys_prev = block->phys_prev;
      nb->phys_prev->phys_next = nb;
      block->phys_prev = nb;

      insert_block(mem_idx, nb);
    }
    block->offset += added_bytes;
    block->size -= added_bytes;
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
	remain_block->device_memory_block = block->device_memory_block;
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

Allocator::block_ptr_t Allocator::merge_insert(block_ptr_t b)
{
  if(auto next_block = b->phys_next; next_block && next_block->free)
  {
    assert(next_block->mem_handle == b->mem_handle);
    assert(next_block->device_memory_block == b->device_memory_block);
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

    // Update phyiscal list
		prev_block->phys_next = b->phys_next;

		if(b->phys_next)
			b->phys_next->phys_prev = prev_block;

		remove_free_list(prev_block);
    
    // Adjust block size and offset
    prev_block->size += b->size;

    m_bag.release(b);
		
		b = prev_block;
  }

	return b;
}
  
void Allocator::remove_free_list(block_ptr_t removed)
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
}

void Allocator::free(block_ptr_t b)
{
  b = merge_insert(b);

	if(b->size == c_largest_block_size)
	{
		free_device_block(b->device_memory_block);
		m_bag.release(b);
	}
	else
		insert_block(b->mem_idx, b);
}

void Allocator::free_device_block(device_block_ptr_t block)
{
	if(block == m_first_device_block)
		m_first_device_block = block->next;
	else
		block->prev->next = block->next;

	if(block->next)
		block->next->prev = block->prev;

	vkFreeMemory(m_device, block->mem_handle, nullptr);
	m_device_blocks.release(block);
}

char * Allocator::map(block_ptr_t block)
{
	auto dev_block = block->device_memory_block;
	if(dev_block->mapped_addr)
	{
		dev_block->n_mapped++;
	}
	else
	{
		vk_check(vkMapMemory(m_device, dev_block->mem_handle, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&dev_block->mapped_addr)));
		dev_block->n_mapped = 1;
	}

	return dev_block->mapped_addr + block->offset;
}

void Allocator::unmap(block_ptr_t block)
{
	auto dev_block = block->device_memory_block;

	if(dev_block->n_mapped == 1)
	{
		vkUnmapMemory(m_device, dev_block->mem_handle);
		dev_block->mapped_addr = nullptr;
	}
	else
		dev_block->n_mapped--;
}

void Allocator::flush_inner(block_ptr_t block)
{
	auto atom_mask = m_nonCoherentAtomSize - 1;
	auto inv_atom_mask = ~atom_mask;

	VkDeviceSize flush_len = block->size + (block->offset & atom_mask);
	VkDeviceSize flush_ofs = block->offset & inv_atom_mask;

	if(flush_len & atom_mask)
		flush_len = (flush_len & inv_atom_mask) + m_nonCoherentAtomSize;

	VkMappedMemoryRange mr{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr,
		block->mem_handle,
		flush_ofs,
		flush_len
	};

	vk_check(vkFlushMappedMemoryRanges(m_device, 1, &mr));
}

void Allocator::invalidate_inner(block_ptr_t block)
{
	auto atom_mask = m_nonCoherentAtomSize - 1;
	auto inv_atom_mask = ~atom_mask;

	VkDeviceSize invalidate_len = block->size + (block->offset & atom_mask);
	VkDeviceSize invalidate_ofs = block->offset & inv_atom_mask;

	if(invalidate_len & atom_mask)
		invalidate_len = (invalidate_len & inv_atom_mask) + m_nonCoherentAtomSize;

	VkMappedMemoryRange mr{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr,
		block->mem_handle,
		invalidate_ofs,
		invalidate_len
	};

	vk_check(vkInvalidateMappedMemoryRanges(m_device, 1, &mr));
}

}
