#include "objects.h"
#include "vkutil.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

void Buffer::create_buffer(VkPhysicalDevice pdev, VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags memprops)
{
	VkBufferCreateInfo bi{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		bs,
		use,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	vk_check(vkCreateBuffer(m_dev, &bi, nullptr, &m_buffer))
	
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(m_dev, m_buffer, &mr);

	VkMemoryAllocateInfo mai{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(pdev, mr, memprops)
	};

	vk_check(vkAllocateMemory(m_dev, &mai, nullptr, &m_mem))

	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem, 0))
}

void CommandBuffer::allocate_command_buffer()
{
	VkCommandBufferAllocateInfo ai{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		m_cmd_pool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};

	vk_check(vkAllocateCommandBuffers(m_dev, &ai, &m_cmd_buf))
}

void CommandBuffer::begin()
{
	VkCommandBufferBeginInfo bi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};

	vk_check(vkBeginCommandBuffer(m_cmd_buf, &bi))
}
