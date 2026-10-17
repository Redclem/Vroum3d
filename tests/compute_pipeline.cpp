

#include "compute/pipeline.h"
#include "core/objects.h"
#include "core/update_descriptor_set.hpp"
#include "debug.h"

#include <vroum3d.h>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace UpdateDescriptorSet;
using namespace Vroum3d;

int main(void)
{
  Display disp;
  DisplayInstance inst(disp);

  PipelineResource pr(inst);

  Compute::Pipeline pipe(pr, Compute::PipelineInformation(pr, "zero.comp.spv"));

  DescriptorPool pool(inst, 32, {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64}});
  Buffer buf(inst, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  uint32_t* buf_data = reinterpret_cast<uint32_t*>(buf.map());
  *buf_data = 0xFFFFFFFF;

  buf.flush();

  VkDescriptorSet s = pool.allocate_descriptor_set(pipe.descriptor_set_layout(0));

  update_descriptor_sets(inst.device(), WriteBuffers<>({{s, 0, 0}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {{buf.buffer(), 0, VK_WHOLE_SIZE}}}));

  VkCommandPool cmdp;

  VkCommandPoolCreateInfo cpi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, inst.compute_queue_index()};

  vk_check(vkCreateCommandPool(inst.device(), &cpi, nullptr, &cmdp));

  CommandBuffer cmd_buf(inst, cmdp);

  cmd_buf.begin_primary();

  cmd_buf.cmd(vkCmdBindPipeline, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.pipeline());
  cmd_buf.cmd(vkCmdBindDescriptorSets, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.layout(), 0, 1, &s, 0, nullptr);
  cmd_buf.cmd(vkCmdDispatch, 1, 1, 1);

  cmd_buf.end();
  inst.quick_submit(cmd_buf, inst.compute_queue());

  cmd_buf.destroy();

  buf.invalidate();

  check(*buf_data == 0);

  vkDestroyCommandPool(inst.device(), cmdp, nullptr);
  buf.unmap();

  return 0;
}
