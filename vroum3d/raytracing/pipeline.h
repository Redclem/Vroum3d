#ifndef VROUM3D_RAYTRACING_PIPELINE_H_INCLUDED
#define VROUM3D_RAYTRACING_PIPELINE_H_INCLUDED

#include "../core/pipeline.h"
#include "../volk_wrapper.h"

#include <vulkan/vulkan_core.h>

namespace Vroum3d::Raytracing
{

class BasePipelineInformation {
public:
  using PipelineResource = Core::PipelineResource;

  struct Shader
  {
    const char* name;
    VkShaderStageFlagBits stage;
  };

  BasePipelineInformation(PipelineResource &pr) : m_pr(pr) {}

  VkDeferredOperationKHR deferred_op() const { return VK_NULL_HANDLE; }

  std::vector<VkPipelineShaderStageCreateInfo> &build_stages() const;

  auto &get_stages() const { return m_stages.empty() ? build_stages() : m_stages; }

  std::vector<VkRayTracingShaderGroupCreateInfoKHR> get_shader_groups() const { return {}; }

  uint32_t get_max_recursion_depth() const { return 0; }

  VkPipelineLibraryCreateInfoKHR get_library_info() const { return {}; }

  VkRayTracingPipelineInterfaceCreateInfoKHR get_interface_info() const { return {}; }

  VkPipelineCreateFlags flags() const { return 0; }

  VkPipeline base_pipeline_handle() const { return VK_NULL_HANDLE; }

  std::int32_t base_pipeline_index() const { return 0; }

protected:
  PipelineResource &m_pr;

  mutable std::vector<VkPipelineShaderStageCreateInfo> m_stages;
};

class TrianglePipelineInformation : public BasePipelineInformation {
public:
  struct Shader
  {
    const char* name;
    VkShaderStageFlagBits stage;
  };

  const PipelineResource::ShaderModule *m_rgen_shader, *m_rmiss_shader;
  const PipelineResource::ShaderModule *m_closest_hit_shader = nullptr, *m_any_hit_shader = nullptr, *m_callable_shader = nullptr;

  void set_closest_hit_shader(std::string_view name) { m_closest_hit_shader = &m_pr.require_shader(name); }

  void set_any_hit_shader(std::string_view name) { m_any_hit_shader = &m_pr.require_shader(name); }

  void set_callable_shader(std::string_view name) { m_callable_shader = &m_pr.require_shader(name); }

  TrianglePipelineInformation(PipelineResource &pr, std::string_view rgen_name, std::string_view rmiss_name) :
    BasePipelineInformation(pr), m_rgen_shader(&pr.require_shader(rgen_name)), m_rmiss_shader(&pr.require_shader(rmiss_name))
  {}

  VkDeferredOperationKHR deferred_op() const { return VK_NULL_HANDLE; }

  std::vector<VkRayTracingShaderGroupCreateInfoKHR> get_shader_groups() const;

  uint32_t get_max_recursion_depth() const { return 0; }

  VkPipelineLibraryCreateInfoKHR get_library_info() const { return {}; }

  VkRayTracingPipelineInterfaceCreateInfoKHR get_interface_info() const { return {}; }

  VkPipelineCreateFlags flags() const { return 0; }

  VkPipeline base_pipeline_handle() const { return VK_NULL_HANDLE; }

  std::int32_t base_pipeline_index() const { return 0; }

  std::vector<VkPipelineShaderStageCreateInfo> &build_stages() const;

  auto &get_stages() const { return m_stages.empty() ? build_stages() : m_stages; }
};

class Pipeline : public Core::BasePipeline {
private:
  using base_pipeline_t = Core::Pipeline;

public:
  template<typename PipelineInformation>
  Pipeline(Core::PipelineResource &pr, PipelineInformation &&pi) : Core::BasePipeline(pr.device())
  {
    create_pipeline(pr, std::forward<PipelineInformation>(pi));
  }

  template<typename PipelineInformation>
  void create_pipeline(Core::PipelineResource &pr, PipelineInformation &&pi);
};

template<typename PipelineInformation>
void Pipeline::create_pipeline(Core::PipelineResource &pr, PipelineInformation &&pi)
{
  m_layouts = pi.get_layouts();

  VkRayTracingPipelineCreateInfoKHR rtpi;

  rtpi.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  rtpi.pNext = nullptr;
  rtpi.flags = pi.flags();

  const std::vector<VkPipelineShaderStageCreateInfo> &stages = pi.get_stages();

  rtpi.stageCount = stages.size();
  rtpi.pStages = stages.data();

  std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups = pi.get_shader_groups();
  rtpi.groupCount = groups.size();
  rtpi.pGroups = groups.data();

  VkPipelineLibraryCreateInfoKHR lib;
  VkRayTracingPipelineInterfaceCreateInfoKHR interface;

  if(rtpi.flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR)
  {
    lib = pi.get_library_info();
    rtpi.pLibraryInfo = &lib;

    interface = pi.get_interface_info();
    rtpi.pLibraryInterface = &interface;
  }
  else
    rtpi.pLibraryInfo = nullptr;

  std::array<VkDynamicState, 2> ds{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dsi{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, std::uint32_t(ds.size()), ds.data()};

  rtpi.pDynamicState = &dsi;
  rtpi.basePipelineHandle = pi.base_pipeline_handle();
  rtpi.basePipelineIndex = pi.base_pipeline_index();

  rtpi.layout = layout();

  vk_check(vkCreateRayTracingPipelinesKHR(m_device, VK_NULL_HANDLE, pr.cache(), 1, &rtpi, nullptr, &m_pipeline));
}

}; // namespace Vroum3d::Raytracing

#endif
