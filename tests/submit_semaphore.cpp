#include "core/objects.h"

#include <vroum3d.h>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

int main()
{
  Instance inst;

  Semaphore sem(inst, true);

  CommandPool cmdp(inst, inst.graphic_queue_index());
  CommandBuffer cmdb0(inst, cmdp);
  CommandBuffer cmdb1(inst, cmdp);
  cmdb0.begin_primary();
  cmdb0.end();
  cmdb1.begin_primary();
  cmdb1.end();

  inst.submit(inst.graphics_queue(), VK_NULL_HANDLE,
              SubmitInfo({{sem, 1, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}, {{cmdb0}}, {{sem, 2, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}),
              SubmitInfo({{sem, 2, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}, {{cmdb1}}, {{sem, 3, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}));

  sem.signal(1);
  sem.wait(3);
  return 0;
}
