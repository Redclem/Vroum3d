#ifndef VROUM3D_CORE_OBJECTS_H_INCLUDED
#define VROUM3D_CORE_OBJECTS_H_INCLUDED

#include "instance.h"
#include "../utility.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

/** Class for a simple buffer, owns the buffer */
class Buffer : public AssignDestroy<Buffer>
{
public:

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
		m_buffer.destroy_with([&](auto buf){vkDestroyBuffer(m_dev, buf, nullptr);});
	  m_alloc->free(m_mem);
  }

	void* map()
	{
		void* ptr;
		vk_check(vkMapMemory(m_dev, m_mem.memory(), 0, VK_WHOLE_SIZE, 0, &ptr));
		return ptr;
	}

	void flush_unmap()
	{
		// TODO : maybe remove unused flush if memory is cached?
		VkMappedMemoryRange mr{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			m_mem.memory(),
			0,
			VK_WHOLE_SIZE
		};

		vk_check(vkFlushMappedMemoryRanges(m_dev, 1, &mr));

		vkUnmapMemory(m_dev, m_mem.memory());
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

	~CommandBuffer() {destroy();}

	CommandBuffer(const Instance& inst, VkCommandPool cmd_pool, bool primary = true) : m_dev(inst.device()), m_cmd_pool(cmd_pool)
	{
		allocate_command_buffer(primary);
	}

	CommandBuffer(const Instance& inst, bool primary = true) : CommandBuffer(inst, inst.transfer_pool(), primary) {}

	void begin_primary();

	void begin_secondary_rendering(Instance& inst);

	void end()
	{
		vkEndCommandBuffer(m_cmd_buf);
	}

	template<auto cmdfun, typename ... Args>
	void cmd(Args&& ... ags)
	{
		cmdfun(m_cmd_buf, std::forward<Args>(ags)...);
	}

	const VkCommandBuffer & cmd_buf() const {return m_cmd_buf;}
	operator VkCommandBuffer() const {return cmd_buf();}

	void begin_rendering(Instance& inst, std::uint32_t idx);

	void end_rendering(Instance& inst, std::uint32_t idx);

	void bind_graphics_pipeline(Instance& inst, VkPipeline pipe);

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

}

#endif
