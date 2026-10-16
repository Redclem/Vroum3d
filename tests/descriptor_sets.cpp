
#include "core/objects.h"
#include "core/update_descriptor_set.hpp"
#include <vroum3d.h>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace UpdateDescriptorSet;

int main(void)
{
  Display disp;
  DisplayInstance inst(disp);

  PipelineResource pr(inst);

  Vroum3d::Core::Pipeline pipe(pr, RenderPipelineInformation(pr, "simple.vert.spv", "simple.frag.spv", {{0}}, {{0, 0}}));

  DescriptorPool pool(inst, 32, {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64}});
  Buffer buf(inst, 128, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0);

  VkDescriptorSet s = pool.allocate_descriptor_set(pipe.descriptor_set_layout(0));

  update_descriptor_sets(inst.device(),
    WriteBuffers<>({{s, 0, 0}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {{buf.buffer(), 0, 128}}})
  );

  return 0;
}


