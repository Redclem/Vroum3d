#include "../vroum3d/vroum3d.h"

using namespace Vroum3d::Core;

int main(int, char*[])
{
	Display disp;
	Instance inst(disp);

	PipelineResource pm(inst);

	return 0;
}
