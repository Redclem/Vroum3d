#include "../vroum3d/vroum3d.h"

using namespace Vroum3d::Core;

int VROUM3D_MAIN()
{
	Display disp;
	DisplayInstance inst(disp);

	PipelineResource pr(inst);

	Pipeline pipe(pr, RenderPipelineInformation(pr, "empty.vert.spv", "empty.frag.spv", {}, {}));

	return 0;
}
