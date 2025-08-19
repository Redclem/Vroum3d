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
	* \param inst Instance to create buffer with
	* \param use Buffer usage
	* \param bs Buffer size
	* \param mp Memory properties of the memory to bind to the buffer
	*/

	Buffer(const Instance& inst, VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags mp) : m_dev(inst.device()) {
		create_buffer(inst.pdev(), bs, use, mp);
	}

	void destroy()
	{
		m_buffer.destroy_with([&](auto buf){vkDestroyBuffer(m_dev, buf, nullptr);});
		m_mem.destroy_with([&](auto mem){vkFreeMemory(m_dev, mem, nullptr);});
	}

	void* map()
	{
		void* ptr;
		vk_check(vkMapMemory(m_dev, m_mem, 0, VK_WHOLE_SIZE, 0, &ptr))
		return ptr;
	}

	void flush_unmap()
	{
		// TODO : maybe remove unused flush if memory is cached?
		VkMappedMemoryRange mr{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			m_mem,
			0,
			VK_WHOLE_SIZE
		};

		vk_check(vkFlushMappedMemoryRanges(m_dev, 1, &mr))

		vkUnmapMemory(m_dev, m_mem);
	}

	~Buffer() {destroy();}

	VkBuffer buffer() const {return m_buffer;}
	operator VkBuffer() const {return buffer();}
private:
	void create_buffer(VkPhysicalDevice pdev, VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags memprops);


	VkHandle<VkBuffer> m_buffer;
	VkHandle<VkDeviceMemory> m_mem;
	VkDevice m_dev;
};

class CommandBuffer : public AssignDestroy<CommandBuffer>
{
public:

	~CommandBuffer() {destroy();}

	CommandBuffer(const Instance& inst, VkCommandPool cmd_pool) : m_dev(inst.device()), m_cmd_pool(cmd_pool)
	{
		allocate_command_buffer();
	}

	CommandBuffer(const Instance& inst) : CommandBuffer(inst, inst.transfer_pool()) {}

	void begin();
	void end()
	{
		vkEndCommandBuffer(m_cmd_buf);
	}

	template<auto cmdfun, typename ... Args>
	void cmd(Args&& ... ags)
	{
		cmdfun(m_cmd_buf, std::forward<Args>(ags)...);
	}

	VkCommandBuffer cmd_buf() const {return m_cmd_buf;}
	operator VkCommandBuffer() const {return cmd_buf();}
private:

	void allocate_command_buffer();

	VkDevice m_dev;
	VkCommandPool m_cmd_pool;
	VkHandle<VkCommandBuffer> m_cmd_buf;

	void destroy()
	{
		m_cmd_buf.destroy_with([&](auto buf){vkFreeCommandBuffers(m_dev, m_cmd_pool, 1, &buf);});
	}
};

}

#endif
