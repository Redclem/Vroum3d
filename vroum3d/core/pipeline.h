#ifndef VROUM3D_CORE_PIPELINE_H_INCLUDED
#define VROUM3D_CORE_PIPELINE_H_INCLUDED

#include "instance.h"
#include "../utility.h"
#include <algorithm>
#include <set>
#include <string_view>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <spirv_reflect.h>

#include <map>

namespace Vroum3d::Core
{

/* Manages shaders, layouts, descriptor set layouts, .... */
class PipelineResource : public AssignDestroy<PipelineResource>
{
	VkHandle<VkPipelineCache> m_cache;
	VkDevice m_device;
	VkFormat m_color_format, m_depth_format;
public:
	struct ShaderModule
	{
		VkHandle<VkShaderModule> vk_mod;
		SpvReflectShaderModule spv_module{};
	};

	struct DescriptorSetDescription
	{
		struct BindingInfo
		{
			VkDescriptorType descriptorType;
			std::uint32_t descriptorCount;
			VkShaderStageFlags stageFlags;
		};

		std::map<std::uint32_t, BindingInfo> bindings;

		void load_descriptor_set(const SpvReflectDescriptorSet*, VkShaderStageFlags);

		void add_binding(std::uint32_t binding, VkDescriptorType tpe, std::uint32_t descriptorCount, VkShaderStageFlags stages)
	{
		bindings.emplace(binding, BindingInfo{tpe, descriptorCount, stages});
	}

		struct Less
		{
			bool operator()(const DescriptorSetDescription& a, const DescriptorSetDescription& b) const
			{
				auto sizecmp = a.bindings.size() <=> b.bindings.size();

				if(0 != sizecmp) return sizecmp < 0;
				

				for(auto itera = a.bindings.begin(), iterb = b.bindings.begin(); itera != a.bindings.end(); itera++, iterb++)
				{
					auto res = memcmp(&*itera, &*iterb, sizeof(*itera));
					if(res) return res < 0;
				}
				return false;
			}
		};

		struct Equal
		{
			bool operator()(const DescriptorSetDescription& a, const DescriptorSetDescription& b) const
			{
				if(a.bindings.size() != b.bindings.size()) return false;

				for(auto itera = a.bindings.begin(), iterb = b.bindings.begin(); itera != a.bindings.end(); itera++, iterb++)
				{
					auto res = memcmp(&*itera, &*iterb, sizeof(*itera));
					if(res) return false;
				}
				return true;
			}
		};
	};

	using shader_module_t = ShaderModule;
	using shaders_t = std::map<std::string, shader_module_t>;
private:
	shaders_t m_shaders;

	
	using descriptor_set_description_t = DescriptorSetDescription;
	using descriptor_set_layouts_t = std::map<DescriptorSetDescription, VkHandle<VkDescriptorSetLayout>, DescriptorSetDescription::Less>;

	descriptor_set_layouts_t m_descriptor_set_layouts;

	struct PipelineLayoutDescription
	{
		std::vector<VkDescriptorSetLayout> layouts;
		std::vector<VkPushConstantRange> pranges;

		struct Less
		{
			bool operator()(const PipelineLayoutDescription& a, const PipelineLayoutDescription& b) const
			{
				if(auto cmp_res = a.layouts <=> b.layouts; 0 != cmp_res) return cmp_res < 0;
				
				if(auto res = memcmp(
					a.pranges.data(),
					b.pranges.data(),
					std::min(a.pranges.size(), b.pranges.size()) * sizeof(a.pranges.front()));
					res)
					return res < 0;

				return a.pranges.size() < b.pranges.size();
			}
		};
	};

	using pipeline_layout_description_t = PipelineLayoutDescription;
	using pipeline_layouts_t = std::map<PipelineLayoutDescription, VkHandle<VkPipelineLayout>, PipelineLayoutDescription::Less>;

	pipeline_layouts_t m_pipeline_layouts;

public:
	VkDevice device() const {return m_device;}

	constexpr static const char * cache_pth = "vk_pipeline_cache";
	PipelineResource(Instance& inst) : m_device(inst.device()), m_color_format(inst.color_format()),
		m_depth_format(inst.depth_format())
	{
		init_cache();
	}

	const VkFormat& color_format() const {return m_color_format;}
	const VkFormat& depth_format() const {return m_depth_format;}

	void destroy()
	{
		write_cache();

		m_cache.destroy_with([&](auto cache){vkDestroyPipelineCache(m_device, cache, nullptr);});

		for(auto& [_, mod] : m_shaders)
		{
			if(VK_NULL_HANDLE != mod.vk_mod)
				vkDestroyShaderModule(m_device, mod.vk_mod, nullptr);
			spvReflectDestroyShaderModule(&mod.spv_module);
		}

		m_shaders.clear();

		for(auto& [_, dsl] : m_descriptor_set_layouts)
		{
			if(VK_NULL_HANDLE != dsl)
				vkDestroyDescriptorSetLayout(m_device, dsl, nullptr);
		}

		m_descriptor_set_layouts.clear();

		for(auto& [_, pl] : m_pipeline_layouts)
		{
			if(VK_NULL_HANDLE != pl)
				vkDestroyPipelineLayout(m_device, pl, nullptr);
		}
	}

	~PipelineResource()
	{
		destroy();
	}

	VkDescriptorSetLayout get_descriptor_set_layout(DescriptorSetDescription&&);

	template<typename ... Shaders>
	VkPipelineLayout get_shader_layouts(Shaders&& ... shaders);

	const ShaderModule& require_shader(std::string_view path);

	VkPipelineCache cache() const {return m_cache;}

private:
	void init_cache();

	void write_cache();
};

/** Base pipeline holder class */
class Pipeline : public AssignDestroy<Pipeline>
{
public:
	template<typename PipelineInformation>
	Pipeline(PipelineResource& pr, const PipelineInformation& pi) : m_device(pr.device())
	{
		create_pipeline(pr, pi);
	}

	template<typename PipelineInformation>
	void create_pipeline(PipelineResource &pr, PipelineInformation& pi);

	void destroy()
	{
		m_pipeline.destroy_with([&](auto pipe){vkDestroyPipeline(m_device, pipe, nullptr);});
	}

	~Pipeline()
	{
		destroy();
	}
private:
	VkDevice m_device;
	VkHandle<VkPipeline> m_pipeline;
};

}



/*******************************//**
  * Implems of template functions
***********************************/

using namespace Vroum3d::Core;

template<typename ... Shaders>
VkPipelineLayout PipelineResource::get_shader_layouts(Shaders&& ... shaders)
{

	std::vector<DescriptorSetDescription> descriptions;
	std::map<std::pair<std::uint32_t, std::uint32_t>, VkShaderStageFlags> push_consts;

	std::array<const ShaderModule*, sizeof...(shaders)> shad_array({&shaders...});

	for(const ShaderModule* smod : shad_array)
	{
		std::uint32_t n_ds;
		spvr_check(spvReflectEnumerateDescriptorSets(&smod->spv_module, &n_ds, nullptr));
		VkShaderStageFlags stage = static_cast<VkShaderStageFlags>(smod->spv_module.shader_stage);

		if(n_ds != 0)
		{
			std::vector<SpvReflectDescriptorSet*> ds(n_ds);
			spvr_check(spvReflectEnumerateDescriptorSets(&smod->spv_module, &n_ds, ds.data()));

			std::uint32_t max_set(0);

			for(auto* desc : ds)
				max_set = std::max(max_set, desc->set);

			if(descriptions.size() <= max_set)
				descriptions.resize(max_set + 1);

			for(auto* desc_set : ds)
			{
				descriptions[desc_set->set].load_descriptor_set(desc_set,
					stage);
			}
		}

		for(auto pc_iter = smod->spv_module.push_constant_blocks,
			pc_end = smod->spv_module.push_constant_blocks + smod->spv_module.push_constant_block_count; pc_iter != pc_end; ++pc_iter)
		{
			auto [iter, ins] = push_consts.emplace(std::pair{pc_iter->offset, pc_iter->size}, stage);
			if(!ins) iter->second |= stage;
		}
	}

	std::vector<VkDescriptorSetLayout> layouts(descriptions.size());

	std::transform(descriptions.begin(), descriptions.end(), layouts.begin(),
	 [&](DescriptorSetDescription& des) {return get_descriptor_set_layout(std::move(des));}
	);

	std::vector<VkPushConstantRange> ranges(push_consts.size());

	std::transform(push_consts.begin(), push_consts.end(), ranges.begin(),
	 [](const auto& pc) -> VkPushConstantRange {
	 return {
		pc.second,
		pc.first.first,
		pc.first.second};
	 });

	auto [iter, wasins] = m_pipeline_layouts.emplace(
		PipelineLayoutDescription{std::move(layouts), std::move(ranges)},
		VK_NULL_HANDLE);

	if(!wasins) return iter->second;

	VkPipelineLayoutCreateInfo pli{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		std::uint32_t(iter->first.layouts.size()),
		iter->first.layouts.data(),
		std::uint32_t(iter->first.pranges.size()),
		iter->first.pranges.data()
	};

	vk_check(vkCreatePipelineLayout(m_device, &pli, nullptr, &iter->second));

	return iter->second;
}

template<typename PipelineInformation>
void Pipeline::create_pipeline(PipelineResource& pr, PipelineInformation& pi)
{
	VkGraphicsPipelineCreateInfo gpi;
	gpi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	gpi.pNext = nullptr;
	gpi.flags = 0;

	const auto& shad_stages = pi.get_stages();
	gpi.stageCount = shad_stages.size();
	gpi.pStages = shad_stages.data();

	const auto& vbds = pi.get_vertex_binding_descriptions();
	const auto& vads = pi.get_vertex_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo visi{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		std::uint32_t(vbds.size()),
		vbds.data(),
		std::uint32_t(vads.size()),
		vads.data()
	};

	gpi.pVertexInputState = &visi;
	
	VkPipelineInputAssemblyStateCreateInfo pias{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		pi.get_primitive_topology(),
		pi.get_primitive_restart_enable()
	};

	gpi.pInputAssemblyState = &pias;

	VkPipelineTessellationStateCreateInfo tsi{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		nullptr,
		0,
		0
	};

	gpi.pTessellationState = &tsi;

	VkPipelineViewportStateCreateInfo vsi{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		nullptr,
		1,
		nullptr
	};

	gpi.pViewportState = &vsi;

	/*! TODO:  More settings for rasterization and default values
	    *  \todo  More settings for rasterization and default values
	    */
	VkPipelineRasterizationStateCreateInfo rsi{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};

	gpi.pRasterizationState = &rsi;

	VkPipelineMultisampleStateCreateInfo msi{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		0.0f,
		nullptr,
		VK_FALSE,
		VK_FALSE
	};

	gpi.pMultisampleState = &msi;

	VkPipelineDepthStencilStateCreateInfo dssi{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		VK_FALSE,
		VK_FALSE,
		{}, {},
		0.0f,
		1.0f
	};

	gpi.pDepthStencilState = &dssi;

	VkPipelineColorBlendAttachmentState cbas{};
	cbas.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo cbsi{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_LOGIC_OP_NO_OP,
		1,
		&cbas,
		{0}
	};

	gpi.pColorBlendState = &cbsi;

	std::array<VkDynamicState, 2> ds{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dsi{
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		nullptr,
		0,
		std::uint32_t(ds.size()),
		ds.data()
	};

	gpi.pDynamicState = &dsi;

	gpi.layout = pi.get_layout();
	gpi.renderPass = VK_NULL_HANDLE;
	gpi.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineRenderingCreateInfo pri{
		VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		nullptr,
		0,
		1,
		&pr.color_format(),
		pr.depth_format(),
		VK_FORMAT_UNDEFINED
	};

	gpi.pNext = &pri;

	vk_check(vkCreateGraphicsPipelines(m_device, pr.cache(), 1, &gpi, nullptr, &m_pipeline));
}	

#endif
