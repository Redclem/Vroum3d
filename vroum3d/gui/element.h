#ifndef VROUM3D_GUI_ELEMENT_H_INCLUDED
#define VROUM3D_GUI_ELEMENT_H_INCLUDED

#include "base.h"

#include <cstdint>
#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <vulkan/vulkan_core.h>


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

class Element
{
	friend class Base;

protected:
	Base* m_base;
	Rect m_position;
	VkDeviceSize m_buffer_offset;
	Element* m_next_element = nullptr;
	
public:

	virtual void set_buffer_offset(VkDeviceSize offset) {m_buffer_offset = offset;}

	Element(Base* base) : m_base(base) {
		m_next_element = base->m_first_elem;
		base->set_first_elem(this);
	}

	Base* base() const {return m_base;}

	/** Get required buffer size for this element (should be constant or at least fixed after init */
	virtual VkDeviceSize get_buffer_size() const = 0;

	/** Init Element : 
	 * - Require needed textures from base using require_texture */
	virtual void init() = 0;

	virtual void record_upl_commands(VkCommandBuffer cmd_buf) = 0;

	const Rect& position() const {return m_position;}
	void set_position(Point p) {static_cast<Point&>(m_position) = p;}
};

class Menu : public Element
{
	std::vector<std::unique_ptr<Element>> m_children;
public:
	using Element::Element;

	virtual VkDeviceSize get_buffer_size() const override;


	virtual void set_buffer_offset(VkDeviceSize buffer_offset) override;
};

}

#endif
