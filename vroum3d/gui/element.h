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
	Element* m_next_element;
	
public:

	auto next_element() const {return m_next_element;}

	void set_buffer_offset(VkDeviceSize offset) {m_buffer_offset = offset;}

	constexpr Element(Base* base) : m_base(base) {
	}

	Base* base() const {return m_base;}

	constexpr static VkDeviceSize c_buffer_size = 0;

	/** Get required buffer size for this element
	 * Does not include child elements!
	 * Should be constant or at least fixed after init */
	constexpr virtual VkDeviceSize buffer_size() const {return c_buffer_size;};

	/** Init Element : 
	 * - Call ancestor's init function (including if deriving directly from Element !)
	 * - Init child elements
	 * - Require needed textures from base using require_texture
	 * - Build required vk objects
	 */
	virtual void init();

	/** Record upload commands for data upload on initialization / size change
	 * Do not call on child elements
	 * \param buffer_data_ptr Pointer to area of memory mapped to buffer. Does not account of offset of current element.
	 */
	virtual void upload_buffer(char * buffer_data_ptr) = 0;

	/** Update inner state on position change.
	 * Should arrange child elements / elements contained */
	virtual void arrange();

	const Rect& position() const {return m_position;}
	void set_position(const Rect& p) {m_position = p;}

	/** Record render commands in given struct
	 * Also record appropriate child commands */
	virtual void record_render_commands(RenderCommands& rc) = 0;

};

class Frame : public Element
{
	px_t m_border, m_margin;

	struct RenderData
	{
		std::array<Point, 10> points;
	};

public:
	auto border() const {return m_border;}
	auto margin() const {return m_margin;}

	void set_border(px_t b) {m_border = b;}
	void set_margin(px_t m) {m_margin = m;}

	Frame(Base* base, px_t border = 0, px_t margin = 0) : Element(base), m_border(border), m_margin(margin) {}

	constexpr static VkDeviceSize c_buffer_size = sizeof(RenderData);
	constexpr virtual VkDeviceSize buffer_size() const override {return c_buffer_size;}
	virtual void init() override;
	virtual void upload_buffer(char * buffer_data_ptr) override;

	virtual void record_render_commands(RenderCommands& rc) override;
};

}

#endif
