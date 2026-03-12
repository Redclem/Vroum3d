#ifndef VROUM3D_CORE_PIPELINE_INFORMATION_HPP_INCLUDED
#define VROUM3D_CORE_PIPELINE_INFORMATION_HPP_INCLUDED

#include "pipeline.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

/** Basic pipeline information class for vertex shader + fragment shader pipeline */
class BasicPipelineInformation
{
	struct BindingDescription
	{
		std::uint32_t stride = 0;
		VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	};

	struct AttributeDescription {
		std::uint32_t binding;
		std::uint32_t offset;
	};

public:
	BasicPipelineInformation(PipelineResource& pr, const char* vs, const char* fs,
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts
	) : m_pr(pr),
		m_vs(pr.require_shader(vs)), m_fs(pr.require_shader(fs)),
		m_stages(build_stages())
	{
		build_input_attachments(binds, atts);
	}

	void build_input_attachments(
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts);

	std::vector<VkPipelineShaderStageCreateInfo> build_stages();
private:
	PipelineResource& m_pr;
	const PipelineResource::ShaderModule &m_vs, &m_fs;

	std::vector<VkVertexInputBindingDescription> m_bindings;
	std::vector<VkVertexInputAttributeDescription> m_attributes;

	std::vector<VkPipelineShaderStageCreateInfo> m_stages;

public:
	const auto& get_stages() const {return m_stages;}
	const auto& get_vertex_binding_descriptions() const {return m_bindings;}
	const auto& get_vertex_attribute_descriptions() const {return m_attributes;}

	VkPrimitiveTopology get_primitive_topology() const
	{
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	VkBool32 get_primitive_restart_enable() const {return VK_FALSE;}

	auto get_layouts() const
	{
		return m_pr.get_shader_layouts(m_vs, m_fs);
	}
};

}

#endif
