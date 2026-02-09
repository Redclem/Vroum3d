#include "../vroum3d/vroum3d.h"

using namespace Vroum3d::Core;

int main()
{
	Display disp;
	Instance inst(disp);

	PipelineResource pr(inst);

	Pipeline pipe(pr, BasicPipelineInformation(pr, "empty.vert.spv", "empty.frag.spv", {}, {}));

	return 0;
}
