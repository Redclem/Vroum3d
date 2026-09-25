#ifndef VROUM3D_GUI_COMMON_H_INCLUDED
#define VROUM3D_GUI_COMMON_H_INCLUDED

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>
#include <array>

namespace Vroum3d::Gui
{

using px_t = float;

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

using rect_points_t = std::array<Point, 4>;

struct Rect : Point, Extent {
	
	constexpr Rect(px_t _x, px_t _y, px_t _w, px_t _h) : Point(_x, _y), Extent(_w, _h) {}
	constexpr Rect(std::uint32_t _x, std::uint32_t _y, std::uint32_t _w, std::uint32_t _h) : Point(_x, _y), Extent(_w, _h) {}
	constexpr Rect(Point p, Extent e) : Point(p), Extent(e) {}
	constexpr Rect() {}

	void shrink(px_t amount)
	{
		if(amount * 2 <= w)
			x += amount, w -= 2 * amount;
		else
		{
			x += w / 2;
			w = 0;
		}

		if(amount * 2 <= h)
			y += amount, h -= 2 * amount;
		else
		{
			y += h / 2;
			h = 0;
		}
	}

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

	// Export triangle points in order : {up right, up left, down left, down right}
	rect_points_t rect_points() const
	{
		return {{{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}};
	}
};


struct RenderCommands
{
	struct Fill
	{
		VkDeviceSize buffer_ofs, n_vertex;
	};

	struct Textured
	{
		VkDeviceSize buffer_ofs, n_vertex;
		std::uint32_t texture_id;
	};

  struct Text
  {
    VkDeviceSize buffer_ofs, n_vertex;
  };

	std::vector<Fill> fills;
	std::vector<Textured> textures;
  std::vector<Text> texts;

	template<typename ... Args>
	void fill(Args&&... ags) {fills.emplace_back(std::forward<Args>(ags)...);}

	template<typename ... Args>
	void texture(Args&&... ags) {textures.emplace_back(std::forward<Args>(ags)...);}

  template<typename ... Args>
  void text(Args&&... ags) {texts.emplace_back(std::forward<Args>(ags)...);}

	void clear()
	{
		fills.clear();
		textures.clear();
    texts.clear();
	}
};

}

#endif
