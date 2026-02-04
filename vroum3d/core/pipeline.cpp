#include "pipeline.h"
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

void PipelineManager::init_cache()
{
	std::ifstream cache_file(cache_pth, std::ios::ate);
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

	vk_check(vkCreatePipelineCache(m_device, &ci, nullptr, &m_cache))
}

void PipelineManager::write_cache()
{
	size_t cache_size;

	if(VK_SUCCESS != vkGetPipelineCacheData(m_device, m_cache, &cache_size, nullptr)) return;
	
	if(cache_size == 0) return;

	std::ofstream cache_file(cache_pth);

	if(!cache_file.is_open()) return;

	std::vector<char> cache_data(cache_size);

	if(VK_SUCCESS != vkGetPipelineCacheData(m_device, m_cache, &cache_size, cache_data.data())) return;

	cache_file.write(cache_data.data(), cache_data.size());
}

VkShaderModule PipelineManager::require_shader(std::string_view path)
{
	auto [iter, ins] = m_shaders.emplace(path, shader_module_t{});

	if(!ins) return iter->second;

	std::string pth("shaders/");
	pth += path;

	std::ifstream sh_file(pth.c_str(), std::ios::ate);
	if(!sh_file.is_open())
		throw std::runtime_error("Could not open shader file : " + pth);

	std::size_t shader_size = sh_file.tellg();
	std::vector<std::uint32_t> shader_data((shader_size + 3) / 4);

	sh_file.seekg(0, std::ios::beg);
	sh_file.read(reinterpret_cast<char*>(shader_data.data()), shader_size);

	sh_file.close();


}
