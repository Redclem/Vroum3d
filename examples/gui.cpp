
#include "../vroum3d/vroum3d.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>

using namespace Vroum3d::Core;
using namespace Vroum3d::Gui;

int main(int, char*[])
{
	Display disp;
	DisplayInstance inst(disp);
	PipelineResource pr(inst);

	Base b(inst, pr);

	bool run(true);
	while(run)
	{
		SDL_Event evnt;
		while(SDL_PollEvent(&evnt))
		{
			if(evnt.type == SDL_EVENT_QUIT)
			{
				run = false;
				break;
			}
		}
		
		if(inst.render_done())
			b.render();
	}

	return 0;
}
