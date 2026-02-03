#ifndef VROUM3D_CORE_PIPELINE_H_INCLUDED
#define VROUM3D_CORE_PIPELINE_H_INCLUDED

#include "instance.h"
#include "../utility.h"
#include <string_view>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <map>

namespace Vroum3d::Core
{

/* Manages shaders, layouts, descriptor set layouts, .... */
class PipelineManager : public AssignDestroy<PipelineManager>
{
	VkHandle<VkPipelineCache> m_cache;
	VkDevice m_device;


	using shader_module_t = VkHandle<VkShaderModule>;
	using shaders_t = std::map<std::string, shader_module_t>;

	shaders_t m_shaders;

public:
	constexpr static const char * cache_pth = "vk_pipeline_cache";
	PipelineManager(Instance& inst) : m_device(inst.device()) {}
private:
	void init_cache();

	void write_cache();

	void destroy()
	{
		write_cache();

		m_cache.destroy_with([&](auto cache){vkDestroyPipelineCache(m_device, cache, nullptr);});

		for(auto& [_, mod] : m_shaders)
			vkDestroyShaderModule(m_device, mod, nullptr);

		m_shaders.clear();
	}

	~PipelineManager()
	{
		destroy();
	}

	VkShaderModule require_shader(std::string_view path);
};

class Pipeline
{
public:
	


private:
};

}

#endif
