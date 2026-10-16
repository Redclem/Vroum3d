
#include "core/pipeline.h"
#include "core/pipeline_info.h"
#include <vroum3d.h>

using namespace Vroum3d::Core;

int main(void)
{
  Display disp;
  DisplayInstance inst(disp);

  PipelineResource pr(inst);

  Vroum3d::Core::Pipeline pipe(pr, RenderPipelineInformation(pr, "simple.vert.spv", "simple.frag.spv", {{0}}, {{0, 0}}));

  return 0;
}


