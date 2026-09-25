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
#include <cwchar>
#include <locale>
#include <string>
#include <string_view>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Gui;

struct FillPipeInfo : public RenderPipelineInformation
{
	FillPipeInfo(PipelineResource& pr) : RenderPipelineInformation(pr, "gui_simple.vert.spv", "gui_fill.frag.spv",
				{{sizeof(Point)}}, {{0, 0}}) {}

	auto get_primitive_topology() const {return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;}
};

struct TexturedPipeInfo : public RenderPipelineInformation
{
	TexturedPipeInfo(PipelineResource& pr) : RenderPipelineInformation(pr, "gui_uv.vert.spv", "gui_textured.frag.spv",
				{{sizeof(TexturedPoint)}}, {{0, 0}, {0, sizeof(Point)}}) {}

	auto get_primitive_topology() const {return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;}
};

struct TextPipeInfo : public RenderPipelineInformation
{
	TextPipeInfo(PipelineResource& pr) : RenderPipelineInformation(pr, "gui_uv.vert.spv", "gui_text.frag.spv",
				{{sizeof(TexturedPoint)}}, {{0, 0}, {0, sizeof(Point)}}) {}

	auto get_primitive_topology() const {return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;}
  VkBool32 get_primitive_restart_enable() const {return VK_TRUE;}

  std::array<VkPipelineColorBlendAttachmentState, 1> blend_attachment_states() const
  {
    VkPipelineColorBlendAttachmentState cbas = {
			VK_TRUE,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_ONE,
			VK_BLEND_OP_ADD,
			VK_COLOR_COMPONENT_R_BIT | 
      VK_COLOR_COMPONENT_G_BIT | 
      VK_COLOR_COMPONENT_B_BIT | 
      VK_COLOR_COMPONENT_A_BIT
		};
    
    return {cbas};
  }
};

Base::Base(DisplayInstance& inst, PipelineResource& pr) : m_instance(&inst), m_pipe_res(&pr), m_device(inst.device()),
	m_fill_pipe(pr, 
		FillPipeInfo(pr)
	),
	m_textured_pipe(pr, TexturedPipeInfo(pr)),
  m_text_pipe(pr, TextPipeInfo(pr))
{
	init_command_buffers();
}

void Base::destroy()
{
	instance()->wait_renders_done();

	m_desc_pool.destroy_with([&](VkDescriptorPool dp){vkDestroyDescriptorPool(device(), dp, nullptr);});
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

  for(auto& [_, font] : m_fonts)
  {
    font.view.destroy_with([&](auto v){vkDestroyImageView(m_device, v, nullptr);});
    font.img.destroy_with([&](auto im){vkDestroyImage(m_device, im, nullptr);});
    m_instance->free(font.mem);
  }
}

void Base::init()
{
	if(m_root_elem)
		m_root_elem->init();

	/** Gather texture and buffer memory requirements */

	std::vector<StbiPtr> textures;
  std::vector<font_bitmap_t> bitmaps;

	VkDeviceSize textures_size = load_textures(textures);
  VkDeviceSize fonts_size = load_font_bitmaps(bitmaps);

  VkDeviceSize buffer_upl_size = textures_size + fonts_size;
		
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
		Instance::c_frames_in_flight * m_buffer_size,
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

	CommandBuffer upl_cmd(*m_instance);
	upl_cmd.begin_primary();
  
  VkDeviceSize ofs(0);

  write_texture_upload_commands(upl_cmd, textures, dt, buf, ofs);
  ofs += textures_size;
  write_font_upload_commands(upl_cmd, bitmaps, dt, buf, ofs);

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

	create_descriptor_set();
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

	m_cmd_bufs.resize(Instance::c_frames_in_flight);

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

	Math::vec2 twice_inv_size = 2.0f / Math::vec2(instance()->w(), instance()->h());
	VkViewport vp{0, 0, float(instance()->w()), float(instance()->h()), 0.0, 1.0};
	VkRect2D scissor{{0, 0}, {instance()->w(), instance()->h()}};

  struct Pc
  {
    Math::vec4 color;
    Math::vec2 twice_inv_size;
    std::uint32_t texture_index;
  } pc;
		pc.twice_inv_size = twice_inv_size;

	if(m_render_commands.fills.size())
	{
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fill_pipe.pipeline());

		pc.color = {0, 0, 0, 1};

		vkCmdPushConstants(cmd_buf, m_fill_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

		vkCmdSetViewport(cmd_buf, 0, 1, &vp);
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

		for(auto& cmd : m_render_commands.fills)
		{
			cmd.buffer_ofs += buffer_ofs;
			vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_buffer, &cmd.buffer_ofs);
			vkCmdDraw(cmd_buf, cmd.n_vertex, 1, 0, 0);
		}
	}

	if(m_render_commands.textures.size())
	{
		pc.color = {1, 1, 1, 1};

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_pipe.pipeline());
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_pipe.layout(), 0, 1, &m_tex_des_set, 0, nullptr);

		vkCmdSetViewport(cmd_buf, 0, 1, &vp);
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
		
		vkCmdPushConstants(cmd_buf, m_textured_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Pc), &pc);

		for(auto& cmd : m_render_commands.textures)
		{
		  vkCmdPushConstants(cmd_buf, m_textured_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 24, sizeof(pc.texture_index), &pc.texture_index);

			cmd.buffer_ofs += buffer_ofs;
			vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_buffer, &cmd.buffer_ofs);
			vkCmdDraw(cmd_buf, cmd.n_vertex, 1, 0, 0);
		}
	}

  if(m_render_commands.texts.size())
  {
    pc.color = {0, 0, 0, 1};
    pc.twice_inv_size = twice_inv_size;

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_text_pipe.pipeline());
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_text_pipe.layout(), 0, 1, &m_tex_des_set, 0, nullptr);

		vkCmdSetViewport(cmd_buf, 0, 1, &vp);
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
		
		vkCmdPushConstants(cmd_buf, m_text_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Pc), &pc);

    for(auto& cmd : m_render_commands.texts)
    {
      cmd.vertex_buffer_ofs += buffer_ofs;
		  vkCmdPushConstants(cmd_buf, m_text_pipe.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 24, sizeof(pc.texture_index), &pc.texture_index);

      vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_buffer, &cmd.vertex_buffer_ofs);
      vkCmdBindIndexBuffer(cmd_buf, m_buffer, cmd.index_buffer_ofs, VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(cmd_buf, cmd.n_vertex, 1, 0, 0, 0);
    }
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

void Base::create_descriptor_set()
{
	std::uint32_t n_tex(m_textures.size() + m_fonts.size());

	if(n_tex == 0) return;

	std::array<VkDescriptorPoolSize, 2> sizes = {{
		{
			VK_DESCRIPTOR_TYPE_SAMPLER,
			1
		},
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			Core::PipelineResource::c_variable_binding_max_size	
		},
	}};

	VkDescriptorPoolCreateInfo dpi{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		1,
		sizes.size(),
		sizes.data()
	};

	vk_check(vkCreateDescriptorPool(device(), &dpi, nullptr, &m_desc_pool));

	auto dsl = m_textured_pipe.descriptor_set_layout(0);
	VkDescriptorSetVariableDescriptorCountAllocateInfo vdcai{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
		nullptr,
		1,
		&n_tex
	};

	VkDescriptorSetAllocateInfo ai{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		&vdcai,
		m_desc_pool,
		1,
		&dsl
	};

	vk_check(vkAllocateDescriptorSets(device(), &ai, &m_tex_des_set));

	std::vector<VkDescriptorImageInfo> img_infos(n_tex);

	std::uint32_t index(0);
	std::transform(m_textures.begin(), m_textures.end(), img_infos.begin(),
								[&index](texture_container_t::value_type& tex) -> VkDescriptorImageInfo
								{
									tex.second.set_index = index++;
									return {VK_NULL_HANDLE, tex.second.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
								});

	std::transform(m_fonts.begin(), m_fonts.end(), img_infos.begin() + m_textures.size(),
								[&index](font_container_t::value_type& font) -> VkDescriptorImageInfo
								{
									font.second.set_index = index++;
									return {VK_NULL_HANDLE, font.second.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
								});

	VkWriteDescriptorSet w{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		m_tex_des_set,
		1,
		0,
		std::uint32_t(img_infos.size()),
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		img_infos.data(),
		nullptr,
		nullptr
	};

	vkUpdateDescriptorSets(device(), 1, &w, 0, nullptr);
}

VkDeviceSize Base::load_textures(std::vector<StbiPtr>& textures)
{
  textures.reserve(m_textures.size());

	VkDeviceSize buffer_upl_size = 0;

	constexpr int chan = 4;
	constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	for(auto& [name, tex] : m_textures)
	{
		int channels;
		int w, h;
		textures.emplace_back(stbi_load(name.c_str(), &w, &h, &channels, chan));
		tex.w = w;
		tex.h = h;

		buffer_upl_size += tex.w * tex.h * chan;

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


    VkImageViewCreateInfo vnfo{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,
      tex.img,
      VK_IMAGE_VIEW_TYPE_2D,
      tex.format,
      vkutil::components_id,
      vkutil::color_subres_plain
    };

    vk_check(vkCreateImageView(m_device, &vnfo, nullptr, &tex.view));
	}

  return buffer_upl_size;
}


void Base::write_texture_upload_commands(CommandBuffer& cmd_buffer, const std::vector<StbiPtr>& textures, char* dt, VkBuffer upl_buffer, VkDeviceSize ofs)
{
  dt += ofs;
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
		1,
		nullptr
	};

  constexpr uint32_t chan = 4;

	{
		auto mri = textures.begin();

		for(auto& [_, tex] : m_textures)
		{
			
			std::uint64_t size = tex.w * tex.h * chan;

			std::memcpy(dt, mri->get(), size);

			VkBufferImageCopy bic{
				ofs,
				tex.w,
				tex.h,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				{0, 0, 0},
				{tex.w, tex.h, 1}
			};

			dt += size;
			ofs += size;
			imb[0].image = imb[1].image = tex.img;

			di.pImageMemoryBarriers = &imb[0];
			cmd_buffer.cmd<vkCmdPipelineBarrier2>(&di);

			cmd_buffer.cmd<vkCmdCopyBufferToImage>(upl_buffer, tex.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);

			di.pImageMemoryBarriers = &imb[1];
			cmd_buffer.cmd<vkCmdPipelineBarrier2>(&di);
			
			mri++;
		}
	}
}


VkDeviceSize Base::load_font_bitmaps(std::vector<Base::font_bitmap_t>& bitmaps)
{
  bitmaps.reserve(m_fonts.size());
  VkDeviceSize size(0);

  constexpr VkFormat format = VK_FORMAT_R8_UNORM;

  for(auto& [name, font] : m_fonts)
  {
    Font f(name.c_str());
    auto [bmp, atlas] = f.render_char_atlas();

    font.glyphs = std::move(atlas);
    
    VkImageCreateInfo imnfo{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			format,
			{bmp.w(), bmp.h(), 1},
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

		vk_check(vkCreateImage(m_device, &imnfo, nullptr, &font.img));

		VkMemoryRequirements imr;
		vkGetImageMemoryRequirements(m_device, font.img, &imr);

		font.mem = m_instance->allocate(imr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vk_check(vkBindImageMemory(device(), font.img, font.mem.memory(), font.mem.offset()));


    VkImageViewCreateInfo vnfo{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,
      font.img,
      VK_IMAGE_VIEW_TYPE_2D,
      format,
      vkutil::components_id,
      vkutil::color_subres_plain
    };

    vk_check(vkCreateImageView(m_device, &vnfo, nullptr, &font.view));

    size += bmp.w() * bmp.h();

    bitmaps.push_back(std::move(bmp));
  }

  return size;
}

void Base::write_font_upload_commands(CommandBuffer& cmd_buffer, const std::vector<Base::font_bitmap_t>& bitmaps, char* dt, VkBuffer upl_buffer, VkDeviceSize ofs)
{
  dt += ofs;
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
		1,
		nullptr
	};

	{
		auto mri = bitmaps.begin();

		for(auto& [_, font] : m_fonts)
		{
			std::uint64_t size = mri->w() * mri->h();

			std::memcpy(dt, mri->data(), size);

			VkBufferImageCopy bic{
				ofs,
				mri->w(),
				mri->h(),
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				{0, 0, 0},
				{mri->w(), mri->h(), 1}
			};

			dt += size;
			ofs += size;
			imb[0].image = imb[1].image = font.img;

			di.pImageMemoryBarriers = &imb[0];
			cmd_buffer.cmd<vkCmdPipelineBarrier2>(&di);

			cmd_buffer.cmd<vkCmdCopyBufferToImage>(upl_buffer, font.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);

			di.pImageMemoryBarriers = &imb[1];
			cmd_buffer.cmd<vkCmdPipelineBarrier2>(&di);
			
			mri++;
		}
	}
}


