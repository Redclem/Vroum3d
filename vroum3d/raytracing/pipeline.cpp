#include "pipeline.h"

#include <vulkan/vulkan_core.h>
using namespace Vroum3d::Raytracing;

std::vector<VkPipelineShaderStageCreateInfo> &TrianglePipelineInformation::build_stages() const
{
  auto add_shader = [&](const PipelineResource::ShaderModule &mod, VkShaderStageFlagBits stg) {
    m_stages.emplace_back(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, stg, mod.vk_mod, "main", nullptr);
  };

  add_shader(*m_rgen_shader, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  add_shader(*m_rmiss_shader, VK_SHADER_STAGE_MISS_BIT_KHR);
  if(m_closest_hit_shader)
    add_shader(*m_closest_hit_shader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  if(m_any_hit_shader)
    add_shader(*m_any_hit_shader, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
  if(m_callable_shader)
    add_shader(*m_callable_shader, VK_SHADER_STAGE_CALLABLE_BIT_KHR);

  return m_stages;
}

std::vector<VkRayTracingShaderGroupCreateInfoKHR> TrianglePipelineInformation::get_shader_groups() const
{
  uint32_t idx(2);
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> res;

  res.emplace_back(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, 0, VK_SHADER_UNUSED_KHR,
                   VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, nullptr);

  res.emplace_back(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, 1, VK_SHADER_UNUSED_KHR,
                   VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, nullptr);

  if(m_callable_shader)
    res.emplace_back(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, idx++, VK_SHADER_UNUSED_KHR,
                     VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, nullptr);

  if(m_closest_hit_shader || m_any_hit_shader)
  {
    VkRayTracingShaderGroupCreateInfoKHR gi{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                            nullptr,
                                            VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                            VK_SHADER_UNUSED_KHR,
                                            VK_SHADER_UNUSED_KHR,
                                            VK_SHADER_UNUSED_KHR,
                                            VK_SHADER_UNUSED_KHR,
                                            nullptr};

    if(m_closest_hit_shader)
      gi.closestHitShader = idx++;
    if(m_any_hit_shader)
      gi.anyHitShader = idx++;

    res.push_back(gi);
  }

  return res;
}
