#ifndef VROUM3D_GUI_ELEMENT_H_INCLUDED
#define VROUM3D_GUI_ELEMENT_H_INCLUDED

#include "common.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vulkan/vulkan_core.h>


namespace Vroum3d::Gui
{

class Base;

class Element
{
	friend class Base;

protected:
	Base* m_base;
	Rect m_position;
	VkDeviceSize m_buffer_offset;
	Element* m_next_element = nullptr;
	
public:

	void set_buffer_offset(VkDeviceSize offset) {m_buffer_offset = offset;}

	constexpr Element(Base* base) : m_base(base) {
	}

	Base* base() const {return m_base;}

	/** Get required buffer size for this element
	 * Does not include child elements!
	 * Should be constant or at least fixed after init */
	constexpr virtual VkDeviceSize buffer_size() const {return 0;};

	/** Init Element : 
	 * - Init child elements
	 * - Require needed textures from base using require_texture
	 * - Build required vk objects
	 * - Init child elements
	 */
	virtual void init();

	virtual void record_upl_commands(VkCommandBuffer cmd_buf);
	virtual void arrange() {}

	const Rect& position() const {return m_position;}
	void set_position(const Rect& p) {m_position = p;}

};

class Frame : public Element
{
	px_t m_border, m_margin;

	struct RenderData
	{
		std::array<Point, 4> margin_out, margin_in;
	};

public:
	auto border() const {return m_border;}
	auto margin() const {return m_margin;}

	void set_border(px_t b) {m_border = b;}
	void set_margin(px_t m) {m_margin = m;}

	Frame(Base* base, px_t border = 0, px_t margin = 0) : Element(base), m_border(border), m_margin(margin) {}

	constexpr virtual VkDeviceSize buffer_size() const override {return 0;}
};

}

#endif
