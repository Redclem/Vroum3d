
#include "../vroum3d/vroum3d.h"
#include "gui/element.h"
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
  b.set_default_font("fonts/font.ttf");

	Label l(&b, "Hello world!");
	b.set_root_elem(&l);
	b.init();

	bool run(true);
	while(run)
	{
		SDL_Event evnt;
		while(SDL_PollEvent(&evnt))
		{
			switch(evnt.type)
			{
			case SDL_EVENT_QUIT:
				run = false;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				b.arrange();
				break;
			}
		}
		if(!run) break;
		
		if(inst.render_done())
			b.render();
	}

	return 0;
}
