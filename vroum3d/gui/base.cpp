#include "base.h"
#include "element.h"

#include "../core/vkutil.h"
#include "../core/objects.h"
#include "../core/pipeline_info.h"
#include "../math/vec.hpp"

#include "../../ext/stb_image.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Gui;

struct FillPipeInfo : public RenderPipelineInformation
{
	FillPipeInfo(PipelineResource& pr) : RenderPipelineInformation(pr, "gui_simple.vert.spv", "gui_fill.frag.spv",
				{{sizeof(Point)}}, {{0, 0}}) {}

	auto get_primitive_topology() const {return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;}
};

Base::Base(DisplayInstance& inst, PipelineResource& pr) : m_instance(&inst), m_pipe_res(&pr), m_device(inst.device()),
	m_fill_pipe(pr, 
		FillPipeInfo(pr)
	)
{
	init_command_buffers();
}

void Base::destroy()
{
	instance()->wait_renders_done();
	m_cmd_pool.destroy_with([&](auto cmdp){vkDestroyCommandPool(m_device, cmdp, nullptr);});

	m_buffer.destroy_with([&](auto buf){vkDestroyBuffer(m_device, buf, nullptr);});

	for(auto& [_, tex] : m_textures)
	{
		tex.view.destroy_with([&](auto v){vkDestroyImageView(m_device, v, nullptr);});
		tex.img.destroy_with([&](auto im){vkDestroyImage(m_device, im, nullptr);});
		m_instance->free(tex.mem);
	}

	m_textures.clear();

	if(m_buffer_mem)
	{
		instance()->unmap(m_buffer_mem);
		instance()->free(m_buffer_mem);
	}
}

void Base::init()
{
	if(m_root_elem)
		m_root_elem->init();

	struct Tptr {
		unsigned char* ptr;

		~Tptr() {if(ptr) std::free(ptr);}
	};

	/** Gather texture and buffer memory requirements */

	std::vector<std::pair<Tptr, VkMemoryRequirements>> textures;
	textures.reserve(m_textures.size());

	VkDeviceSize buffer_upl_size = 0;

	constexpr int chan = 4;

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	for(auto& [name, tex] : m_textures)
	{
		int channels;
		int w, h;
		textures.emplace_back(
			Tptr{stbi_load(name.c_str(), &w, &h, &channels, chan)}, 
			VkMemoryRequirements{});
		tex.w = w;
		tex.h = h;

		buffer_upl_size += tex.w * tex.h * chan;

		// TODO : add direct upload on relevant platforms
		VkImageCreateInfo imnfo{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			format,
			{tex.w, tex.h, 1},
			1,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		vk_check(vkCreateImage(m_device, &imnfo, nullptr, &tex.img));

		VkMemoryRequirements imr;
		vkGetImageMemoryRequirements(m_device, tex.img, &imr);

		tex.mem = m_instance->allocate(imr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vk_check(vkBindImageMemory(device(), tex.img, tex.mem.memory(), tex.mem.offset()));
	}
	
	for(auto elem = m_first_element; elem; elem = elem->next_element())
	{
		elem->set_buffer_offset(m_buffer_size);
		m_buffer_size += elem->buffer_size();
	}

	buffer_upl_size += m_buffer_size;

	VkBufferCreateInfo bnfo{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		c_frames_in_flight * m_buffer_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	vk_check(vkCreateBuffer(m_device, &bnfo, nullptr, &m_buffer));
	
	VkMemoryRequirements bmr;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &bmr);

	m_buffer_mem = m_instance->allocate(bmr, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_mapped_buffer_ptr = instance()->map(m_buffer_mem);

	vk_check(vkBindBufferMemory(device(), m_buffer, m_buffer_mem.memory(), m_buffer_mem.offset()));

	Buffer buf(*m_instance, buffer_upl_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	char* dt = reinterpret_cast<decltype(dt)>(buf.map());

	/** Bind and upload data */

	VkDeviceSize upl_ofs(0);
	CommandBuffer upl_cmd(*m_instance);
	upl_cmd.begin_primary();

	std::array<VkImageMemoryBarrier2, 2> imb{{{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		0,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		0,
		0,
		VK_NULL_HANDLE,
		vkutil::color_subres_plain
	}, {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
		0,
		0,
		VK_NULL_HANDLE,
		vkutil::color_subres_plain
	}}};

	VkDependencyInfo di{
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		std::uint32_t(imb.size()),
		imb.data()
	};

	{
		auto mri = textures.begin();

		for(auto& [_, tex] : m_textures)
		{
			VkImageViewCreateInfo vnfo{
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				nullptr,
				0,
				tex.img,
				VK_IMAGE_VIEW_TYPE_2D,
				format,
				vkutil::components_id,
				vkutil::color_subres_plain
			};

			vk_check(vkCreateImageView(m_device, &vnfo, nullptr, &tex.view));
			
			std::uint64_t size = tex.w * tex.h * chan;

			std::memcpy(dt, mri->first.ptr, size);

			dt += size;
			upl_ofs += size;

			VkBufferImageCopy bic{
				upl_ofs,
				tex.w,
				tex.h,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0},
				{0, 0, 0},
				{tex.w, tex.h, 1}
			};

			upl_cmd.cmd<vkCmdCopyBufferToImage>(buf, tex.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);

			imb[0].image = imb[1].image = tex.img;
			upl_cmd.cmd<vkCmdPipelineBarrier2>(&di);
			
			mri++;
		}
	}

	upl_cmd.end();

	VkSubmitInfo si{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0,
		nullptr,
		nullptr,
		1,
		&upl_cmd.cmd_buf(),
		0,
		nullptr
	};

	Fence fnc(*m_instance);

	vk_check(vkQueueSubmit(m_instance->transfer_queue(), 1, &si, fnc.fence()));

	arrange();

	fnc.wait();
}

void Base::init_command_buffers()
{
	VkCommandPoolCreateInfo pi{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		m_instance->graphic_queue_index()
	};

	vk_check(vkCreateCommandPool(m_device, &pi, nullptr, &m_cmd_pool));

	m_cmd_bufs.resize(c_frames_in_flight);

	VkCommandBufferAllocateInfo cmdai{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		m_cmd_pool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		std::uint32_t(m_cmd_bufs.size())
	};

	vk_check(vkAllocateCommandBuffers(m_device, &cmdai, m_cmd_bufs.data()));
}

void Base::build_render_buffer()
{	
	m_render_commands.clear();

	if(m_root_elem)
	{
		m_root_elem->record_render_commands(m_render_commands);
	}

	VkDeviceSize buffer_ofs(m_buffer_size * instance()->next_frame());
	char * data_ptr = m_mapped_buffer_ptr + buffer_ofs;

	for(auto elem = m_first_element; elem; elem = elem->next_element())
	{
		elem->upload_buffer(data_ptr);
	}

	auto cmd_buf = m_cmd_bufs[instance()->next_frame()];

	vk_check(vkResetCommandBuffer(cmd_buf, 0));

	VkCommandBufferBeginInfo bi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		0,
		nullptr
	};

	vk_check(vkBeginCommandBuffer(cmd_buf, &bi));	
	m_instance->begin_rendering(cmd_buf);

	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fill_pipe.pipeline());

	struct Pc
	{
		Math::vec4 color;
		Math::vec2 twice_inv_size;
	} pc;

	pc.color = {0, 0, 0, 1};
	pc.twice_inv_size = 2.0f / Math::vec2(instance()->w(), instance()->h());

	vkCmdPushConstants(cmd_buf, m_fill_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

	VkViewport vp{0, 0, float(instance()->w()), float(instance()->h()), 0.0, 1.0};
	vkCmdSetViewport(cmd_buf, 0, 1, &vp);

	VkRect2D scissor{{0, 0}, {instance()->w(), instance()->h()}};
	vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

	for(auto& cmd : m_render_commands.fills)
	{
		cmd.buffer_ofs += buffer_ofs;
		vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_buffer, &cmd.buffer_ofs);
		vkCmdDraw(cmd_buf, cmd.n_vertex, 1, 0, 0);
	}

	instance()->end_rendering(cmd_buf);

	vk_check(vkEndCommandBuffer(cmd_buf));
}

void Base::render()
{
	if(!instance()->acquire_next_image()) return;

	build_render_buffer();

	instance()->submit_render_present(m_cmd_bufs[instance()->next_frame()]);
}
