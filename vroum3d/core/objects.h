#ifndef VROUM3D_CORE_OBJECTS_H_INCLUDED
#define VROUM3D_CORE_OBJECTS_H_INCLUDED

#include "instance.h"
#include "../utility.h"
#include <algorithm>
#include <type_traits>
#include <vulkan/vulkan_core.h>
#include "../volk_wrapper.h"

namespace Vroum3d::Core
{

/** Class for a simple buffer, owns the buffer */
class Buffer : public AssignDestroy<Buffer>
{

  static constexpr VkDeviceSize c_max_atom_size = 256;
public:

  Buffer() {}

	/** Buffer constructor : creates buffer and allocs mem
	* \param alloc Allocator to create buffer with
	* \param use Buffer usage
	* \param bs Buffer size
	* \param mp Memory properties of the memory to bind to the buffer
	*/

	Buffer(Allocator& alloc, VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags mp) : m_alloc(&alloc), m_dev(alloc.device()) {
		create_buffer(bs, use, mp);
	}

  Buffer(Buffer&&) = default;

	void destroy()
	{
		m_buffer.destroy_with([&](auto buf){
      vkDestroyBuffer(m_dev, buf, nullptr);
	    m_alloc->free(m_mem);
    });
  }

	char* map()
	{
		return m_alloc->map(m_mem);
	}

	void flush_unmap()
	{
		m_alloc->flush(m_mem);
		m_alloc->unmap(m_mem);
	}

	void unmap()
	{
		m_alloc->unmap(m_mem);
	}

  void invalidate()
  {
    VkDeviceSize offset = m_mem.offset(), size = m_mem.size();

    VkDeviceSize true_ofs = offset & ~(c_max_atom_size - 1);
    VkDeviceSize rectified_size = size + offset - true_ofs;

    if(rectified_size % c_max_atom_size)
    {
      rectified_size = (rectified_size & ~(c_max_atom_size - 1)) + c_max_atom_size;
      rectified_size = std::min(rectified_size, Allocator::c_largest_block_size - true_ofs);
    }

		VkMappedMemoryRange mr{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			m_mem.memory(),
			true_ofs,
			rectified_size
		};

		vk_check(vkInvalidateMappedMemoryRanges(m_dev, 1, &mr));
  }

  VkDeviceAddress device_address() const
  {
    VkBufferDeviceAddressInfoKHR dai{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      nullptr,
      m_buffer
    };

    return vkGetBufferDeviceAddress(m_dev, &dai);
  }

	~Buffer() {destroy();}

	const VkBuffer& buffer() const {return m_buffer;}
	operator VkBuffer() const {return buffer();}
private:
	void create_buffer(VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags memprops);

  Allocator* m_alloc;
	VkDevice m_dev;
	VkHandle<VkBuffer> m_buffer;

  Allocator::owned_memory_t m_mem;
};

class CommandBuffer : public AssignDestroy<CommandBuffer>
{
public:

  CommandBuffer() {}

	~CommandBuffer() {destroy();}

  CommandBuffer(CommandBuffer&&) = default;

	CommandBuffer(const Instance& inst, VkCommandPool cmd_pool, bool primary = true) : m_dev(inst.device()), m_cmd_pool(cmd_pool)
	{
		allocate_command_buffer(primary);
	}

	CommandBuffer(const Instance& inst, bool primary = true) : CommandBuffer(inst, inst.transfer_pool(), primary) {}

	void begin_primary();

	void begin_secondary_rendering(DisplayInstance& inst);

	void end()
	{
		vkEndCommandBuffer(m_cmd_buf);
	}

	template<typename CmdFun, typename ... Args>
	void cmd(CmdFun cf, Args&& ... ags)
	{
		cf(m_cmd_buf, std::forward<Args>(ags)...);
	}

	const VkCommandBuffer & cmd_buf() const {return m_cmd_buf;}
	operator VkCommandBuffer() const {return cmd_buf();}

	void begin_rendering(DisplayInstance& inst)
  {
    inst.begin_rendering(m_cmd_buf);
  }

	void end_rendering(DisplayInstance& inst)
  {
    inst.end_rendering(m_cmd_buf);
  }

	void bind_graphics_pipeline(DisplayInstance& inst, VkPipeline pipe);
	
  void reset()
	{

		vkResetCommandBuffer(m_cmd_buf, 0);
	}

	void destroy()
	{
		m_cmd_buf.destroy_with([&](auto buf){vkFreeCommandBuffers(m_dev, m_cmd_pool, 1, &buf);});
	}
private:

	void allocate_command_buffer(bool primary = true)
	{
		VkCommandBufferAllocateInfo ai{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		m_cmd_pool,
		primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		1
	};

	vk_check(vkAllocateCommandBuffers(m_dev, &ai, &m_cmd_buf));
}

	VkDevice m_dev;
	VkCommandPool m_cmd_pool;
	VkHandle<VkCommandBuffer> m_cmd_buf;
};

class Fence : public AssignDestroy<Fence>
{
public:
  Fence() {}
  
	Fence(const Instance& inst) : m_device(inst.device())
	{
		VkFenceCreateInfo fi{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr,
			0
		};

		vk_check(vkCreateFence(m_device, &fi, nullptr, &m_fence));
	}

	const VkFence& fence() const {return m_fence;}

	auto wait()
	{
		vk_check(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, ~(0)));
	}

	~Fence() {destroy();}

	void destroy()
	{
		m_fence.destroy_with([&](auto fnc){vkDestroyFence(m_device, fnc, nullptr);});
	}

private:
	VkDevice m_device;
	VkHandle<VkFence> m_fence;
};

/** Descriptor pool */
class DescriptorPool : public AssignDestroy<DescriptorPool>
{
public:
  DescriptorPool(Instance& inst, uint32_t max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes) : m_device(inst.device()) {
    VkDescriptorPoolCreateInfo info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      max_sets,
      uint32_t(pool_sizes.size()),
      pool_sizes.data()
    };

    vk_check(vkCreateDescriptorPool(m_device, &info, nullptr, &m_descriptor_pool));
  }

  void destroy()
  {
      m_descriptor_pool.destroy_with([&](auto dp) {vkDestroyDescriptorPool(m_device, dp, nullptr);});
  }

  template<typename ContainerIn, typename ContainerOut>
  void allocate_descriptor_sets(const ContainerIn& layouts, ContainerOut& sets_out, const void* pnext = nullptr)
  {
    VkDescriptorSetAllocateInfo ai{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      pnext,
      m_descriptor_pool,
      uint32_t(layouts.size()),
      layouts.data()
    };

    vk_check(vkAllocateDescriptorSets(m_device, &ai, sets_out.data()));
  }

  template<typename ContainerIn, typename ContainerOut, typename VarSizes,
    std::enable_if_t<!std::is_pointer_v<VarSizes>, bool> = true>
  void allocate_descriptor_sets(const ContainerIn& layouts, ContainerOut& sets_out, const VarSizes& var_sizes)
  {
    VkDescriptorSetVariableDescriptorCountAllocateInfo vsi{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
      nullptr,
      uint32_t(var_sizes.size()),
      var_sizes.data()
    };

    allocate_descriptor_sets(layouts, sets_out, &vsi);
  }

  VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout lay)
  {
    std::array<VkDescriptorSetLayout, 1> lay_cont{{lay}};
    std::array<VkDescriptorSet, 1> out;

    allocate_descriptor_sets(lay_cont, out);

    return out[0];
  }

  VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout lay, uint32_t varsize)
  {
    std::array<uint32_t, 1> varsize_arr{{varsize}};
    std::array<VkDescriptorSetLayout, 1> lay_cont{{lay}};
    std::array<VkDescriptorSet, 1> out;

    allocate_descriptor_sets(lay_cont, out, varsize_arr);

    return out[0];
  }

private:
  VkDevice m_device;
  VkHandle<VkDescriptorPool> m_descriptor_pool;
};

}

#endif
