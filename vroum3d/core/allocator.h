#ifndef VROUM3D_CORE_ALLOCATOR_H_INCLUDED
#define VROUM3D_CORE_ALLOCATOR_H_INCLUDED

#include "../bag.hpp"
#include "vkutil.h"

#include <cstddef>
#include <filesystem>
#include <malloc.h>
#include <memory>
#include <type_traits>
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

#ifdef VROUM3D_ALLOCATOR_DRY_ALLOC
  static constexpr bool c_dry_allocation = true;
#else
  static constexpr bool c_dry_allocation = false;
#endif

  using mem_handle_t = std::conditional_t<c_dry_allocation, std::uint64_t, VkDeviceMemory>;

  static std::uint32_t int_log2(VkDeviceSize x)
  {
    std::uint32_t log(0);
    if(x >= (1ull<<32))
      log |= 32, x >>= 32;
    if(x >= (1ull<<16))
      log |= 16ull, x >>= 16;
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


  static std::uint32_t fsb(uint64_t a)
  {
      int l(0);
      if((a & 0xFFFFFFFF) == 0)
          l |= 32, a >>= 32;
      if((a & 0xFFFF) == 0)
          l |= 16, a >>= 16;
      if((a & 0xFF) == 0)
          l |= 8, a >>= 8;
      if((a & 0xF) == 0)
          l |= 4, a >>= 4;
      if((a & 0x3) == 0)
          l |= 2, a >>= 2;
      if((a & 0x1) == 0)
          l |= 1, a >>= 1;
      return l;
  }

  
  using bit_field_t = std::uint32_t;
  static constexpr std::size_t c_log_largest_block_size = 28, c_log_smallest_block_size = 5;
  static constexpr std::size_t c_largest_block_size = 1 << c_log_largest_block_size, c_smallest_block_size = 1<< c_log_smallest_block_size;

  static constexpr std::size_t c_prim_bin_size = c_log_largest_block_size - c_log_smallest_block_size + 1;
  static constexpr std::size_t c_sec_bin_size = 16;

  static constexpr std::size_t c_n_mem_types = 32;

  static_assert(VK_MAX_MEMORY_TYPES <= c_n_mem_types);

  

private:
	VkDevice m_device = VK_NULL_HANDLE;
	VkPhysicalDevice m_pdev = VK_NULL_HANDLE;


  struct MemBlock
  {
    static constexpr std::size_t c_mem_idx_bit_width = 8;

    static_assert(c_n_mem_types <= (1 << c_mem_idx_bit_width));

    VkDeviceSize size, offset;
    mem_handle_t mem_handle;
    Bag<MemBlock>::ptr_t phys_next = nullptr, phys_prev = nullptr;
    Bag<MemBlock>::ptr_t list_next = nullptr, list_prev = nullptr;
    std::uint32_t mem_idx : c_mem_idx_bit_width;
    bool free : 1 = false; // Should represent at any time if the block is inserted in a free block chain
    // TODO : Add host visible / coherent
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
    
    using addr_t = std::pair<std::uint32_t, std::uint32_t>;

    bit_field_t free = 0;
    std::array<secondary_bin_t, c_prim_bin_size> sec_bins; // index 0 are smallest blocks

    // Address of the list of free blocks that would contain a block of given size
    static addr_t addr(VkDeviceSize size)
    {
      std::uint32_t prim_bin = int_log2(size >> c_log_smallest_block_size);

      VkDeviceSize sec_bin_span, sec_bin_smallest_size;

      if(0 == prim_bin)
      {
        sec_bin_span = c_smallest_block_size << 1;
        sec_bin_smallest_size = 0;
      }
      else {
        sec_bin_span = 1 << (prim_bin + c_log_smallest_block_size);
        sec_bin_smallest_size = sec_bin_span;
      }

      return {prim_bin, c_sec_bin_size * (size - sec_bin_smallest_size) / sec_bin_span};
    }

    auto& head_ptr_at(const addr_t& addr)
    {
      return sec_bins[addr.first].list_heads[addr.second];
    }

    void set_free_flag(const addr_t& addr)
    {
      free |= 1ull << addr.first;
      sec_bins[addr.first].free |= 1ull << addr.second;
    }

    void unset_free_flag(const addr_t& addr)
    {
      sec_bins[addr.first].free &= ~(1ull << addr.second);
      if(!sec_bins[addr.first].free)
        free &= ~(1ull << addr.first);
    }
  };

  using primary_bin_t = PrimaryBin;

  bit_field_t m_free_prim_bins = 0;
  
  using prim_bins_t = std::array<primary_bin_t, c_n_mem_types>;

  prim_bins_t m_prim_bins;
  block_bag_t m_bag;

  using memory_handles_t = std::unordered_set<mem_handle_t>;
  memory_handles_t m_mem_handles;

  // Only call when associated memory index is empty. Returns a new block allocated through vk without inserting it in the pools.
  block_ptr_t allocate_vk_block(std::uint32_t index);

  // Insert block (usually after allocating part of it) in pools
  void insert_block(std::uint32_t mem_idx, block_ptr_t block)
  {
    insert_block(mem_idx, block, PrimaryBin::addr(block->size));
  }

  // Insert block (usually after allocating part of it) in pools
  void insert_block(std::uint32_t mem_idx, block_ptr_t block, PrimaryBin::addr_t addr);

  // Get block from smallest matching pool / list (removes it)
  block_ptr_t get_block(std::uint32_t mem_idx, VkDeviceSize size);

  // Split block (reduce size to given size, returns block following)
  block_ptr_t split_block(VkDeviceSize newsize, block_ptr_t block);

  // Try to merge block with physical neighbors
  void merge(block_ptr_t block);

  // Free block
  void free(block_ptr_t b);
  
  // Allocate Memory
  block_ptr_t allocate_inner(std::uint32_t mem_idx, VkDeviceSize size, VkDeviceSize alignment);

  void remove_free_list(block_ptr_t removed);

public:

  // Raw Allocated Memory handle, exposing block property access
  class AllocatedMemory : block_ptr_t
  {
    friend Allocator;

    using base_t = block_ptr_t;

    AllocatedMemory(block_ptr_t b) : base_t(b) {}

    const base_t& base() const {return static_cast<const block_ptr_t&>(*this);}

    AllocatedMemory(std::nullptr_t) : base_t() {}

    AllocatedMemory& operator=(std::nullptr_t)
    {
      base_t::operator=(nullptr);
      return *this;
    }
  public:
    using base_t::operator bool;

    AllocatedMemory() {}

    mem_handle_t memory() const {return base()->mem_handle;}
    VkDeviceSize offset() const {return base()->offset;}
    VkDeviceSize size() const {return base()->size;}
  };

  using allocated_memory_t = AllocatedMemory;

  /** Allocate memory with given index, size, alignment. Use find_mem_index to easily determine memory index
   * On vulkan exception in memory allocation, state is unchanged.
   * */
  allocated_memory_t allocate(std::uint32_t mem_idx, VkDeviceSize s, VkDeviceSize alignment = 1) {
    check(s <= c_largest_block_size);
    return allocate_inner(mem_idx, s, alignment);
  }

  /** Allocate memory for given requirements with given memory property bits
   */
  allocated_memory_t allocate(const VkMemoryRequirements& mr, VkMemoryPropertyFlagBits mf)
  {
    return allocate_inner(find_mem_index(mr, mf), mr.size, mr.alignment);
  }

  /** Owned memory handle, non copyable. Ensures unique ownership of memory handle and no duplication
   * Similar role to what would be VkHandle<VkDeviceMemory>
   * \warning Does not ensure automatical freeing / release of memory !
   * */
  class OwnedMemory : allocated_memory_t, NonCopyable
  {
    using base_t = allocated_memory_t;

    base_t& base() {return static_cast<base_t&>(*this);}
    const base_t& base() const {return static_cast<const base_t&>(*this);}

    friend Allocator;

  public:
    using base_t::memory, base_t::offset, base_t::size, base_t::operator bool;
    OwnedMemory() {}

    OwnedMemory(OwnedMemory&& from) : allocated_memory_t(std::exchange(from.base(), {})) {}

    OwnedMemory(allocated_memory_t&& am) : allocated_memory_t(am) {}

    OwnedMemory& operator=(allocated_memory_t&& rhs)
    {
      base_t::operator=(rhs);
      return *this;
    }

    OwnedMemory& operator=(OwnedMemory&& rhs)
    {
      base() = std::exchange(rhs.base(), {});
      return *this;
    }

    ~OwnedMemory() {}

    OwnedMemory(std::nullptr_t) : base_t() {}

    OwnedMemory& operator=(std::nullptr_t)
    {
      base_t::operator=(nullptr);
      return *this;
    }
  };

  using owned_memory_t = OwnedMemory;

  // Free memory
  void free(allocated_memory_t am) {
    if(am)
      free(am.base());
  }

  // Free memory
  void free(owned_memory_t& om) {
    if(om)
    {
      free(om.base());
      om = nullptr;
    }
  }

  // Free memory
  void free(const owned_memory_t& om) {
    if(om)
      free(om.base());
  }


	VkDevice device() const{return m_device;}

	VkPhysicalDevice pdev() const {return m_pdev;}

	void destroy();

  std::uint32_t find_mem_index(const VkMemoryRequirements& mr, VkMemoryPropertyFlags props)
  {
    return vkutil::find_mem_index(m_pdev, mr, props);
  }

protected:
	Allocator() {}

	void init(VkDevice dev, VkPhysicalDevice pdev);
};

}

#endif
