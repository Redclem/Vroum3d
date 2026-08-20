
#include "../vroum3d/vroum3d.h"

using namespace Vroum3d::Core;
using namespace Vroum3d::Gui;

int main(int, char*[])
{
	Display disp;
	DisplayInstance inst(disp);
	PipelineResource pr(inst);

	Base b(inst, pr);

	return 0;
}
