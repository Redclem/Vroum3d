#include "pipeline.h"
#include <algorithm>
#include <spirv_reflect.h>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

void PipelineResource::init_cache()
{
	std::ifstream cache_file(cache_pth, std::ios::ate | std::ios::binary);
	std::vector<char> cache_data;
	
	if(cache_file.is_open())
	{
		auto size = cache_file.tellg();
		cache_data.resize(size);

		cache_file.seekg(0, std::ios::beg);
		cache_file.read(cache_data.data(), cache_data.size());
	}
	cache_file.close();

	VkPipelineCacheCreateInfo ci{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		nullptr,
		0,
		cache_data.size(),
		cache_data.data()
	};

	if(vkCreatePipelineCache(m_device, &ci, nullptr, &m_cache) != VK_SUCCESS)
  {
    ci.initialDataSize = 0;
    ci.pInitialData = nullptr;

    vk_check(vkCreatePipelineCache(m_device, &ci, nullptr, &m_cache));
  }
}

void PipelineResource::write_cache()
{
  if(!m_cache) return;

	size_t cache_size;

	if(VK_SUCCESS != vkGetPipelineCacheData(m_device, m_cache, &cache_size, nullptr)) return;
	
	if(cache_size == 0) return;

	std::ofstream cache_file(cache_pth, std::ios::binary);

	if(!cache_file.is_open()) return;

	std::vector<char> cache_data(cache_size);

	if(VK_SUCCESS != vkGetPipelineCacheData(m_device, m_cache, &cache_size, cache_data.data())) return;

	cache_file.write(cache_data.data(), cache_data.size());
}

const PipelineResource::ShaderModule& PipelineResource::require_shader(std::string_view path)
{
	auto [iter, ins] = m_shaders.emplace(path, shader_module_t{});

	if(!ins) return iter->second;

	std::string pth("shaders/");
	pth += path;

	std::ifstream sh_file(pth.c_str(), std::ios::ate | std::ios::binary);
	if(!sh_file.is_open())
		throw std::runtime_error("Could not open shader file : " + pth);

	std::size_t shader_size = sh_file.tellg();
	std::vector<std::uint32_t> shader_data((shader_size + 3) / 4);

  if(shader_data.empty())
    throw std::runtime_error("Empty shader file " + pth);
	shader_data.back() = 0;

	sh_file.seekg(0, std::ios::beg);
	sh_file.read(reinterpret_cast<char*>(shader_data.data()), shader_size);

	sh_file.close();
	
	VkShaderModuleCreateInfo smi{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		shader_data.size() * sizeof(uint32_t),
		shader_data.data()
	};

	vk_check(vkCreateShaderModule(m_device, &smi, nullptr, &iter->second.vk_mod));

	spvr_check(spvReflectCreateShaderModule(shader_data.size() * sizeof(uint32_t), shader_data.data(), &iter->second.spv_module));

	return iter->second;
}

VkDescriptorSetLayout PipelineResource::get_descriptor_set_layout(DescriptorSetDescription&& des)
{
	auto [iter, ins] = m_descriptor_set_layouts.emplace(std::move(des), VK_NULL_HANDLE);

	if(!ins) return iter->second;

  auto& des_ref = iter->first;

	std::vector<VkDescriptorSetLayoutBinding> binds(des_ref.bindings.size());

	std::transform(des_ref.bindings.begin(), des_ref.bindings.end(), binds.begin(),
		[&](const auto& bind_info) -> VkDescriptorSetLayoutBinding
		{	
			VkDescriptorSetLayoutBinding res{
				bind_info.first,
				bind_info.second.descriptorType,
				bind_info.second.descriptorCount,
				bind_info.second.stageFlags,
				bind_info.second.immusamp ? &m_immutable_samplers[0] : nullptr
			};

			if(res.pImmutableSamplers)
			{
				require_immutable_samplers(bind_info.second.descriptorCount);
			}

			return res;
		}
	);

	VkDescriptorSetLayoutCreateInfo dsi{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		std::uint32_t(binds.size()),
		binds.data()
	};

	std::vector<VkDescriptorBindingFlags> flags;
	VkDescriptorSetLayoutBindingFlagsCreateInfo fi;

	constexpr VkDescriptorBindingFlags c_null_desc_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

	if(binds.back().descriptorCount == 0)
	{
		flags.reserve(binds.size());
		flags.assign(binds.size(), 0);
		flags.back() = c_null_desc_flags;

		binds.back().descriptorCount = c_variable_binding_max_size;

		fi = VkDescriptorSetLayoutBindingFlagsCreateInfo{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			nullptr,
			std::uint32_t(flags.size()),
			flags.data()
		};
	}

	vk_check(vkCreateDescriptorSetLayout(m_device, &dsi, nullptr, &iter->second));

	return iter->second;
}

void PipelineResource::DescriptorSetDescription::load_descriptor_set(const SpvReflectDescriptorSet* ds, VkShaderStageFlags stage)
{
	
	for(auto* ds_iter = ds->bindings, *end = ds->bindings + ds->binding_count; ds_iter != end; ++ds_iter)
	{
		auto& ds_binding = **ds_iter;

		auto [iter, ins] = bindings.emplace(
			ds_binding.binding,
			BindingInfo{
			static_cast<VkDescriptorType>(ds_binding.descriptor_type),
			ds_binding.count,
			stage,
			std::strcmp(ds_binding.name, c_immutable_samplers_name) == 0
			}
		);
		if(!ins)
		{
			auto& di = iter->second;
			check(ds_binding.count == di.descriptorCount
				&& static_cast<VkDescriptorType>(ds_binding.descriptor_type) == di.descriptorType
			);

			di.stageFlags |= stage;
		}
	}
}

void PipelineResource::require_immutable_samplers(std::size_t n_samplers)
{
	if(n_samplers > c_n_immutable_samplers)
		throw std::runtime_error("Required more immutable samplers than available");

	for(std::size_t sampler(0); sampler != n_samplers; ++sampler)
	{
		if(m_immutable_samplers[sampler] != VK_NULL_HANDLE) continue;

		vk_check(vkCreateSampler(device(), &c_immutable_sampler_info[sampler], nullptr, &m_immutable_samplers[sampler]));
	}
}
