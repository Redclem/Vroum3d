#include "allocator.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

void Allocator::init(VkDevice dev, VkPhysicalDevice pdev)
{
  m_device = dev;
  m_pdev = pdev;

  
}

void Allocator::allocate_vk_block(std::uint32_t idx)
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

  m_mem_handles.insert(hdl);
  
  m_prim_bins[idx].sec_bins[c_prim_bin_size - 1].list_heads[c_sec_bin_size - 1] = new_block;
  m_prim_bins[idx].sec_bins[c_prim_bin_size - 1].free |= (1ull << (c_sec_bin_size - 1));
  m_prim_bins[idx].free |= (1ull << (c_prim_bin_size - 1));
}

void Allocator::destroy()
{
  for(auto& elem : m_mem_handles)
  {
    vkFreeMemory(m_device, elem, nullptr);
  }
  m_mem_handles.clear();
}

}
