#ifndef VROUM3D_CORE_ALLOCATOR_H_INCLUDED
#define VROUM3D_CORE_ALLOCATOR_H_INCLUDED

#include "../bag.hpp"

#include <unordered_set>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdlib>
#include <cstdint>
#include <array>

namespace Vroum3d::Core
{

class Allocator
{
public:
  static std::uint32_t int_log2(VkDeviceSize x)
  {
    std::uint32_t log(0);
    if(x >= (1ull<<32))
      log |= 32, x >>= 32;
    if(x >= (1ull<<16))
      log |= 1ull6, x >>= 16;
    if(x >= (1ull<<8))
      log |= 8, x >>= 8;
    if(x >= (1ull<<4))
      log |= 4, x >>= 4;
    if(x >= (1ull<<2))
      log |= 2, x >>= 2;
    if(x >= (1ull<<1))
      log |= 1ull, x >>= 1;

    return log;
  }
  
  using bit_field_t = std::uint32_t;
  static constexpr std::size_t c_log_largest_block_size = 20, c_log_smallest_block_size = 5;
  static constexpr std::size_t c_largest_block_size = 1 << c_log_largest_block_size, c_smallest_block_size = 1<< c_log_smallest_block_size;

  static constexpr std::size_t c_prim_bin_size = c_log_largest_block_size - c_log_smallest_block_size + 1;
  static constexpr std::size_t c_sec_bin_size = 16;

  static constexpr std::size_t c_n_mem_types = 32;
private:
	VkDevice m_device = VK_NULL_HANDLE;
	VkPhysicalDevice m_pdev = VK_NULL_HANDLE;


  struct MemBlock
  {
    VkDeviceSize size, offset;
    VkDeviceMemory mem_handle;
    Bag<MemBlock>::ptr_t* next, prev;
  };

  using block_bag_t = Bag<MemBlock>;
  using block_ptr_t = block_bag_t::ptr_t;

  using mem_block_t = MemBlock;

  struct SecondaryBin{
    bit_field_t free = 0;
    std::array<block_ptr_t, c_sec_bin_size> list_heads; // Index 0 are smallest blocks
  };

  using secondary_bin_t = SecondaryBin;

  struct PrimaryBin{
    bit_field_t free = 0;
    std::array<secondary_bin_t, c_prim_bin_size> sec_bins; // index 0 are smallest blocks

    std::pair<std::uint32_t, std::uint32_t> size_idx(VkDeviceSize size)
    {
      int prim_bin = std::max(0, int_log2(size) - c_log_smallest_block_size));

      VkDeviceSize sec_bin_span = prim_bin == 0 ? c_smallest_block_size << 1 :
            1 << (prim_bin + c_log_smallest_block_size);

      return {prim_bin, c_sec_bin_size * size / sec_bin_span};
    }
  };

  using primary_bin_t = PrimaryBin;

  bit_field_t m_free_prim_bins = 0;
  std::array<primary_bin_t, c_n_mem_types> m_prim_bins;
  block_bag_t m_bag;

  using memory_handles_t = std::unordered_set<VkDeviceMemory>;
  memory_handle_t m_mem_handles;

  // Only call when associated memory index is empty
  void allocate_vk_block(std::uint32_t index);

public:

  friend class Instance;

	VkDevice device() const{return m_device;}

	VkPhysicalDevice pdev() const {return m_pdev;}
private:
	Allocator() {}

	void init(VkDevice dev, VkPhysicalDevice pdev);

	void destroy();
};

}

#endif
