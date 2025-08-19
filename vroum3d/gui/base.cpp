#include "base.h"
#include "element.h"
#include "../core/vkutil.h"
#include "../core/objects.h"

#include "../../ext/stb_image.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Gui;

void Base::destroy()
{
	m_buffer.destroy_with([&](auto buf){vkDestroyBuffer(m_dev, buf, nullptr);});

	for(auto& [_, tex] : m_textures)
	{
		if(tex.view != VK_NULL_HANDLE)
			vkDestroyImageView(m_dev, tex.view, nullptr);
		if(tex.img != VK_NULL_HANDLE)
			vkDestroyImage(m_dev, tex.img, nullptr);
	}

	m_textures.clear();

	m_mem.destroy_with([&](auto mem) {vkFreeMemory(m_dev, mem, nullptr);});
}

void Base::init()
{
	for(auto * elem = m_first_elem; elem; elem = elem->m_next_element)
		elem->init();

	struct Tptr {
		char* ptr;

		~Tptr() {if(ptr) std::free(ptr);}
	};

	std::vector<std::pair<Tptr, VkMemoryRequirements>> textures;
	textures.reserve(m_textures.size());

	VkDeviceSize buffer_upl_size = 0;

	VkMemoryRequirements mr{0, 0, 0};

	m_rgb = rgb_supported();
	int chan = m_rgb ? 3 : 4;

	VkFormat format = m_rgb ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;

	for(auto& [name, tex] : m_textures)
	{
		int channels;
		// TODO : add check to see if RGB8 supported and use if supported
		textures.emplace_back(stbi_load(name.c_str(), &tex.w, &tex.h, &channels, chan), VkMemoryRequirements{});

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

		vk_check(vkCreateImage(m_dev, &imnfo, nullptr, &tex.img))

		VkMemoryRequirements imr;
		vkGetImageMemoryRequirements(m_dev, tex.img, &imr);

		vkutil::add_mem_reqs(mr, imr);
	}
	
	std::vector<VkDeviceSize> buffer_sizes;
	VkDeviceSize buf_tot_size = 0;
	
	for(auto iter = m_first_elem; iter; iter = iter->m_next_element)
	{
		auto size = iter->get_buffer_size();
		buffer_sizes.push_back(size);
		buf_tot_size += size;
	}

	buffer_upl_size += buf_tot_size;

	VkBufferCreateInfo bnfo{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		buf_tot_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	vk_check(vkCreateBuffer(m_dev, &bnfo, nullptr, &m_buffer))
	
	VkMemoryRequirements bmr;
	vkGetBufferMemoryRequirements(m_dev, m_buffer, &bmr);

	vkutil::add_mem_reqs(mr, bmr);

	VkMemoryAllocateInfo anfo{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(m_instance->pdev(), mr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	vk_check(vkAllocateMemory(m_dev, &anfo, nullptr, &m_mem))

	Buffer buf(*m_instance, buffer_upl_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	char* dt = reinterpret_cast<decltype(dt)>(buf.map());

	VkDeviceSize ofs(0), upl_ofs(0);
	CommandBuffer upl_cmd(*m_instance);
	upl_cmd.begin();

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
			ofs = vkutil::match_offset(ofs, mri->second.alignment);

			vk_check(vkBindImageMemory(m_dev, tex.img, m_mem, ofs))

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

			vk_check(vkCreateImageView(m_dev, &vnfo, nullptr, &tex.view))
			
			ofs += mri->second.size;

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

	ofs = vkutil::match_offset(ofs, bmr.alignment);
	vk_check(vkBindBufferMemory(m_dev, m_buffer, m_mem, ofs))

	for(auto elem = m_first_elem; elem; elem = elem->m_next_element)
		elem->record_upl_commands(upl_cmd);

	upl_cmd.end();


}

bool Base::rgb_supported()
{
	VkFormatProperties2 fp;
	vkGetPhysicalDeviceFormatProperties2(m_instance->pdev(), VK_FORMAT_R8G8B8_SRGB, &fp);
	return fp.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT;
}
