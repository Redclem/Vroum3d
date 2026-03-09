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

	/*VkMemoryAllocateInfo mai{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(pdev, mr, memprops)
	};

	vk_check(vkAllocateMemory(m_dev, &mai, nullptr, &m_mem));*/

  m_mem = m_alloc->allocate(m_alloc->find_mem_index(mr, memprops), mr.size, mr.alignment);

	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem.memory(), 0));
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

	std::array<VkImageMemoryBarrier2, 2> barriers = {{
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		0,
		0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		0,
		0,
		inst.sw_image(idx),
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	},
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		0,
		0,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		0,
		0,
		inst.depth_image(),
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	}
	}};

	VkDependencyInfo di{
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		barriers.size(),
		barriers.data()
	};

	vkCmdPipelineBarrier2(m_cmd_buf, &di);

	VkRenderingAttachmentInfo
	catt{
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

	VkRenderingAttachmentInfo
	datt{
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		nullptr,
		inst.depth_view(),
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
		VK_RESOLVE_MODE_NONE,
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
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

void CommandBuffer::end_rendering(Instance& inst, std::uint32_t idx)
{
	vkCmdEndRendering(m_cmd_buf);

	VkImageMemoryBarrier2 bar = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		0,
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		0,
		0,
		inst.sw_image(idx),
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};

	VkDependencyInfo di{
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&bar
	};

	vkCmdPipelineBarrier2(m_cmd_buf, &di);
}

void CommandBuffer::bind_graphics_pipeline(Instance& inst, VkPipeline pipe)
{
	vkCmdBindPipeline(m_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

	VkRect2D sc{{0, 0}, {inst.w(), inst.h()}};
	VkViewport vp{
		0.0,
		0.0,
		float(inst.w()),
		float(inst.h()),
		0.0f,
		1.0f
	};

	vkCmdSetViewport(m_cmd_buf, 0, 1, &vp);
	vkCmdSetScissor(m_cmd_buf, 0, 1, &sc);
}
