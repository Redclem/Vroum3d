#ifndef VROUM3D_CORE_DIPLAY_HPP_INCLUDED
#define VROUM3D_CORE_DIPLAY_HPP_INCLUDED

#include "../utility.h"
#include "../debug.h"

#include <SDL3/SDL.h>

namespace Vroum3d::Core
{

class Display : public AssignDestroy<Display>
{
	Handle<SDL_Window*, nullptr> m_wind;
	Handle<SDL_Surface*, nullptr> m_surf;

	friend class Instance;

public:

  Display& display() {return *this;}

	void destroy()
	{
		m_wind.destroy_with(SDL_DestroyWindow);
	}

	Display(const char* title = "Vroum3d app", int w = 1280, int h = 720, int flags = SDL_WINDOW_VULKAN)
	{
		if(SDL_WasInit(SDL_INIT_VIDEO) != SDL_INIT_VIDEO)
			sdl_check(SDL_Init(SDL_INIT_VIDEO));

		m_wind = SDL_CreateWindow(title, w, h, flags);

		check(m_wind);
	}

  SDL_Window* window() const {return m_wind;}
  SDL_Surface* surface() const {return m_surf;}
};

}

#endif
