#ifndef VROUM3D_GUI_ELEMENT_H_INCLUDED
#define VROUM3D_GUI_ELEMENT_H_INCLUDED

#include "common.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
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

	Element(Base* base) : m_base(base) {
	}

	Base* base() const {return m_base;}

	/** Get required buffer size for this element
	 * Does not include child elements!
	 * Should be constant or at least fixed after init */
	virtual VkDeviceSize get_buffer_size() const {return 0;};

	/** Init Element : 
	 * - Init child elements
	 * - Require needed textures from base using require_texture
	 * - Build required vk objects
	 */
	virtual void init();

	/* Register element
	 * - Register this element and children to base (through register_element call on children) */
	virtual void register_element();

	virtual void record_upl_commands(VkCommandBuffer cmd_buf);

	const Rect& position() const {return m_position;}
	void set_position(Point p) {static_cast<Point&>(m_position) = p;}

};

class Menu : public Element
{
	std::vector<Element*> m_children;
public:
	using Element::Element;

	virtual VkDeviceSize get_buffer_size() const override;
};

}

#endif
