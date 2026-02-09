#include "objects.h"
#include "instance.h"
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

void CommandBuffer::begin_rendering(Instance& inst, std::uint32_t idx)
{
	VkRenderingAttachmentInfo
	catt{
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		nullptr,
		inst.sw_view(idx),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_RESOLVE_MODE_NONE,
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_STORE,
		{.depthStencil = {1.0, 1}}
	};

	VkRenderingAttachmentInfo
	datt{
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		nullptr,
		inst.sw_view(idx),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_RESOLVE_MODE_NONE,
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		{.color = {{0}}}
	};
	
	VkRenderingInfo ri{
		VK_STRUCTURE_TYPE_RENDERING_INFO,
		nullptr,
		0,
		{{0, 0}, {inst.w(), inst.h()}},
		1,
		0,
		1,
		&catt,
		&datt,
		nullptr
	};

	vkCmdBeginRendering(m_cmd_buf, &ri);
}
