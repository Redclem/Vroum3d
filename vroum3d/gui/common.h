#ifndef VROUM3D_GUI_COMMON_H_INCLUDED
#define VROUM3D_GUI_COMMON_H_INCLUDED

#include <cstdint>

namespace Vroum3d::Gui
{

using px_t = std::uint32_t;

struct Extent
{
	px_t w = 0, h = 0;

	Extent& operator+=(const Extent& rhs)
	{
		w += rhs.w;
		h += rhs.h;
		return *this;
	}

	Extent& operator-=(const Extent& rhs)
	{
		w -= rhs.w;
		h -= rhs.h;
		return *this;
	}
};

struct Point
{
	px_t x = 0, y = 0;

	Point& operator+=(const Point& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Point& operator-=(const Point& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
};

inline Point operator+(Point lhs, const Point& rhs)
{
	return lhs += rhs;
}

inline Point operator-(Point lhs, const Point& rhs)
{
	return lhs -= rhs;
}

inline Extent operator+(Extent lhs, const Extent& rhs)
{
	return lhs += rhs;
}

inline Extent operator-(Extent lhs, const Extent& rhs)
{
	return lhs -= rhs;
}

struct Rect : Extent, Point {
	Point& origin()
	{
		return static_cast<Point&>(*this);
	}

	const Point& origin() const
	{
		return static_cast<const Point&>(*this);
	}

	Extent& extent()
	{
		return static_cast<Extent&>(*this);
	}

	const Extent& extent() const
	{
		return static_cast<const Extent&>(*this);
	}
};


}

#endif
