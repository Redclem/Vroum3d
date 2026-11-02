#include "core/objects.h"

#include <vroum3d.h>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;

int main()
{
  Instance inst;

  Semaphore sem(inst, true);

  CommandPool cmdp(inst, inst.graphic_queue_index());
  CommandBuffer cmdb(inst, cmdp);
  cmdb.begin_primary();
  cmdb.end();

  inst.submit(inst.graphics_queue(), VK_NULL_HANDLE,
              SubmitInfo({{sem, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}, {{cmdb}}, {{sem, 1, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}}));

  sem.wait(1);
  return 0;
}
