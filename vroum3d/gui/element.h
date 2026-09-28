#ifndef VROUM3D_GUI_ELEMENT_H_INCLUDED
#define VROUM3D_GUI_ELEMENT_H_INCLUDED

#include "common.h"
#include "base.h"

#include "../math/vec.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <vulkan/vulkan_core.h>


namespace Vroum3d::Gui
{

struct TexturedPoint
{
	Point pos;
	Math::vec2 uv;
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
	virtual VkDeviceSize buffer_size() const override {return c_buffer_size;}
	virtual void init() override;
	virtual void upload_buffer(char * buffer_data_ptr) override;

  virtual Extent get_min_dimensions() override {return {m_border + m_margin, m_border + m_margin};}

	virtual void record_render_commands(RenderCommands& rc) override;
};

class Image : public Element
{
	Math::vec2 m_texture_offset, m_texture_extent;

	struct RenderData{
		std::array<TexturedPoint, 4> points;
	};

	Base::texture_ptr_t m_texture;

public:
	Image(Base* base, const char* pth) : Element::Element(base), m_texture_offset(0.0f, 0.0f), m_texture_extent(1.0f, 1.0f), m_texture(base->require_texture(pth))
	{
	}

	constexpr static VkDeviceSize c_buffer_size = sizeof(RenderData);
	virtual VkDeviceSize buffer_size() const override {return c_buffer_size;}

	virtual void init() override;
	virtual void upload_buffer(char* buffer_data_ptr) override;
	virtual void record_render_commands(RenderCommands& rc) override;
	virtual void arrange() override;
};

class Label : public Element
{
  struct GlyphData
  {
    TexturedPoint pts[4];
  };
private:
  std::string m_text;
  Base::font_ptr_t m_font;

  Math::vec2 m_text_orig;

protected:
  std::size_t m_n_glyphs = 0;
public:

  template<typename T>
  Label(Base* base, T&& text) : Element::Element(base), m_text(std::forward<T>(text)),
   m_font(base->default_font()) {}

  virtual VkDeviceSize buffer_size() const override {
    return m_font->glyphs.glyph_count(m_text) * (sizeof(GlyphData) + 5 * sizeof(std::uint16_t)) - sizeof(std::uint16_t);
  }
  
  virtual void init() override;
  virtual void upload_buffer(char* buffer_data_ptr) override;
  virtual void record_render_commands(RenderCommands& rc) override;
  virtual void arrange() override;

  virtual Extent get_min_dimensions() override;
};

class Grid : public Element
{
public:
  struct GridElement
  {
    Element * elem;
    std::uint32_t col, row;
  };

  using grid_element_t = GridElement;
  using grid_elements_t = std::vector<grid_element_t>;
  
  Grid(Base* b) : Element::Element(b) {}

  virtual void init() override;
  virtual VkDeviceSize buffer_size() const override {return 0;}
  virtual void upload_buffer(char *) override {}
  virtual Extent get_min_dimensions() override;
  virtual void arrange() override;
  virtual void record_render_commands(RenderCommands&) override;

  void compute_min_dimensions();

  void add_element(Element* elem, std::uint32_t col, std::uint32_t row) {m_grid_elements.emplace_back(elem, col, row);}

private:

  bool min_dimensions_computed() const {return m_grid_elements.empty() || !m_min_col_width.empty();}

  grid_elements_t m_grid_elements;
  std::vector<px_t> m_min_col_width, m_min_row_height;

  px_t m_min_width, m_min_height;
};

}

#endif
