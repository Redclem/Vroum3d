#ifndef VROUM3D_GUI_COMMON_H_INCLUDED
#define VROUM3D_GUI_COMMON_H_INCLUDED

#include <cstdint>

namespace Vroum3d::Gui
{

using px_t = std::uint32_t;

struct Extent
{
	px_t w, h;
};

struct Point
{
	px_t x, y;
};

struct Rect : Extent, Point {};


}

#endif
