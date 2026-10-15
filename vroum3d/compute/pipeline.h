#ifndef VROUM3D_COMPUTE_PIPELINE_H_INCLUDED
#define VROUM3D_COMPUTE_PIPELINE_H_INCLUDED

#include "../core/pipeline.h"
#include "../core/pipeline_info.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Compute
{

class PipelineInformation 
{
protected:
  PipelineResource& m_pr;
  const PipelineResource::ShaderModule * m_shader;

public:
  PipelineInformation(PipelineResource& pr, const char* shader) : m_pr(pr),
    m_shader(&m_pr.require_shader(shader)) {}



};

class Pipeline : public Core::Pipeline
{
public:
  template<typename PipelineInformation>
  void create_pipeline(PipelineResource& pr, PipelineInformation& pi);

private:
  using base_pipeline_t = Core::Pipeline;
};

template<typename PipelineInformation>
void Pipeline::create_pipeline(PipelineResource& pr, PipelineInformation& pi)
{
  m_layouts = pi.get_layouts();

  VkComputePipelineCreateInfo ci{
    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    nullptr,
    0,
    pi.get_stage(),
    m_layouts.layout(),
    VK_NULL_HANDLE,
    0
  };

  vk_check(vkCreateComputePipelines(m_device, pr.cache(), 1, &ci, nullptr, &m_pipeline));
}

}

#endif
