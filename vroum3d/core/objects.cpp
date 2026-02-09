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

	vk_check(vkCreateBuffer(m_dev, &bi, nullptr, &m_buffer));
	
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(m_dev, m_buffer, &mr);

	VkMemoryAllocateInfo mai{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(pdev, mr, memprops)
	};

	vk_check(vkAllocateMemory(m_dev, &mai, nullptr, &m_mem));

	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem, 0));
}

void CommandBuffer::begin_primary()
{
	VkCommandBufferBeginInfo bi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};

	vk_check(vkBeginCommandBuffer(m_cmd_buf, &bi));
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
		{.color = {{0}}}
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
		{.depthStencil = {1.0, 1}}
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

void CommandBuffer::begin_secondary_rendering(Instance& inst)
{

	VkCommandBufferInheritanceRenderingInfo inhri{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
		nullptr,
		0,
		0,
		1,
		&inst.color_format(),
		inst.depth_format(),
		VK_FORMAT_UNDEFINED,
		VK_SAMPLE_COUNT_1_BIT
	};

	VkCommandBufferInheritanceInfo inhi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		&inhri,
		VK_NULL_HANDLE,
		0,
		VK_NULL_HANDLE,
		VK_FALSE,
		0,
		0
	};

	VkCommandBufferBeginInfo bi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
		| VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		&inhi	
	};

	vk_check(vkBeginCommandBuffer(m_cmd_buf, &bi));
}
