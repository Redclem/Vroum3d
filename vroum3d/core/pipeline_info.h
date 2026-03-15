#ifndef VROUM3D_CORE_PIPELINE_INFORMATION_HPP_INCLUDED
#define VROUM3D_CORE_PIPELINE_INFORMATION_HPP_INCLUDED

#include "pipeline.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

/** Basic graphics pipeline information class, no color output but depth output */
class GraphicsPipelineInformation
{
protected:
	struct BindingDescription
	{
		std::uint32_t stride;
		VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	};

	struct AttributeDescription {
		std::uint32_t binding;
		std::uint32_t offset;
	};

	PipelineResource& m_pr;
  const PipelineResource::ShaderModule &m_vs;

	std::vector<VkVertexInputBindingDescription> m_bindings;
	std::vector<VkVertexInputAttributeDescription> m_attributes;

	mutable std::vector<VkPipelineShaderStageCreateInfo> m_stages;

public:
	GraphicsPipelineInformation(PipelineResource& pr, const char* vs, 
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts
	) : m_pr(pr), m_vs(pr.require_shader(vs))
	{
		build_input_attachments(binds, atts);
	}

	void build_input_attachments(
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts);

	std::vector<VkPipelineShaderStageCreateInfo> build_stages() const;
public:
	const auto& get_stages() const {
    return m_stages = build_stages();
  }

	const auto& get_vertex_binding_descriptions() const {return m_bindings;}
	const auto& get_vertex_attribute_descriptions() const {return m_attributes;}

	VkPrimitiveTopology get_primitive_topology() const
	{
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	VkBool32 get_primitive_restart_enable() const {return VK_FALSE;}

	auto get_layouts() const
	{
		return m_pr.get_shader_layouts(m_vs);
	}

  VkBool32 depth_test_enable() const {return VK_TRUE;}
  VkBool32 depth_write_enable() const {return VK_TRUE;}

  VkCompareOp depth_compare_op() const {return VK_COMPARE_OP_LESS;}

  VkPipeline base_pipeline_handle() const {return VK_NULL_HANDLE;}
  std::int32_t base_pipeline_index() const {return 0;}

  VkBool32 rasterizer_discard_enable() const {return VK_FALSE;}

  
  std::array<VkPipelineColorBlendAttachmentState, 1> blend_attachment_states() const
  {
    return {};
  }

  auto rendering_info() const
  {
    return VkPipelineRenderingCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      nullptr,
      0,
      0,
      nullptr,
      m_pr.depth_format(),
      VK_FORMAT_UNDEFINED
  	};
  }
};

/** Basic pipeline information class for vertex shader + fragment shader pipeline */
class RenderPipelineInformation : public GraphicsPipelineInformation
{
  using base_t = GraphicsPipelineInformation;
protected:
	const PipelineResource::ShaderModule &m_fs;

public:
	RenderPipelineInformation(PipelineResource& pr, const char* vs, const char* fs,
		const std::vector<BindingDescription> &binds,
		const std::vector<AttributeDescription> &atts
	) : base_t(pr, vs, binds, atts),
		m_fs(pr.require_shader(fs))
	{
	}

	std::vector<VkPipelineShaderStageCreateInfo> build_stages() const;

public:

	const auto& get_stages() const {
    return m_stages = build_stages();
  }
	const auto& get_vertex_binding_descriptions() const {return m_bindings;}
	const auto& get_vertex_attribute_descriptions() const {return m_attributes;}

	auto get_layouts() const
	{
		return m_pr.get_shader_layouts(m_vs, m_fs);
	}
  
  std::array<VkPipelineColorBlendAttachmentState, 1> blend_attachment_states() const
  {
    VkPipelineColorBlendAttachmentState cbas;
    cbas.blendEnable = VK_FALSE;
    cbas.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
      VK_COLOR_COMPONENT_G_BIT | 
      VK_COLOR_COMPONENT_B_BIT | 
      VK_COLOR_COMPONENT_A_BIT;
    
    return {cbas};
  }

  auto rendering_info() const
  {
    return VkPipelineRenderingCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      nullptr,
      0,
      1,
      &m_pr.color_format(),
      m_pr.depth_format(),
      VK_FORMAT_UNDEFINED
  	};
  }
};

}

#endif
