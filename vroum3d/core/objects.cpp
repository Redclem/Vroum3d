#include "objects.h"
#include "instance.h"
#include "vkutil.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

void Buffer::create_buffer(VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags memprops)
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

  m_mem = m_alloc->allocate(m_alloc->find_mem_index(mr, memprops), mr.size, mr.alignment);

	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem.memory(), m_mem.offset()));
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

void CommandBuffer::begin_secondary_rendering(DisplayInstance& inst)
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

void CommandBuffer::bind_graphics_pipeline(DisplayInstance& inst, VkPipeline pipe)
{
	vkCmdBindPipeline(m_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

  inst.set_dynamic_viewport_scissor(m_cmd_buf);
}
