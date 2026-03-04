#include "pipeline_info.h"
#include "pipeline.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

void BasicPipelineInformation::build_input_attachments(
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts)
{
	std::uint32_t cnt;
	spvr_check(spvReflectEnumerateInputVariables(&m_vs.spv_module, &cnt, nullptr));

	if(!cnt) return;
	std::vector<SpvReflectInterfaceVariable*> variables(cnt);
	spvr_check(spvReflectEnumerateInputVariables(&m_vs.spv_module, &cnt, variables.data()));

	{
		std::uint32_t binding(0);
		m_bindings.resize(binds.size());
		std::transform(binds.begin(), binds.end(), m_bindings.begin(),
			[&](const BindingDescription& ds) -> VkVertexInputBindingDescription
			{
				return {
					binding++,
					ds.stride,
					ds.input_rate
				};
			}
		);
	}

	{
		std::uint32_t location(0);
		m_attributes.resize(atts.size());
		std::transform(atts.begin(), atts.end(), m_attributes.begin(),
			[&](const AttributeDescription& ds) -> VkVertexInputAttributeDescription
			{
				return {
					location++,
					ds.binding,
					{},
					ds.offset
				};
			}
		);
	}

	for(auto* itf_var : variables)
	{
		if(itf_var->location == std::numeric_limits<decltype(itf_var->location)>::max()) continue;
		m_attributes[itf_var->location].format = static_cast<VkFormat>(itf_var->format);
	}
}

std::vector<VkPipelineShaderStageCreateInfo> BasicPipelineInformation::build_stages()
{
	return {
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_VERTEX_BIT,
			m_vs.vk_mod,
			"main",
			nullptr
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			m_fs.vk_mod,
			"main",
			nullptr
		}
	};
}
