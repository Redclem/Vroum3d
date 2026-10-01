#include "element.h"
#include "../core/vkutil.h"
#include "base.h"
#include "common.h"
#include <cassert>
#include <numeric>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Gui;

void Element::init()
{
	base()->register_element(this);
}

void Element::arrange()
{
}

void Frame::init()
{
	Element::init();
}

void Frame::upload_buffer(char * buffer_data_ptr)
{
  frame_write_buffer(buffer_data_ptr, Rect(position()).shrink(m_margin), m_border);
}

void Frame::record_render_commands(RenderCommands& rc)
{
  frame_render(rc, m_buffer_offset + Element::c_buffer_size);
}

void Image::init()
{
	Element::init();
}

void Image::upload_buffer(char* buffer_data_ptr)
{
	RenderData& rd = *reinterpret_cast<RenderData*>(buffer_data_ptr + Element::c_buffer_size + m_buffer_offset);

	auto points = position().rect_points();

	rd.points = {{
		{points[0], m_texture_offset},
		{points[1], m_texture_offset + Math::vec2{m_texture_extent.x, 0}},
		{points[3], m_texture_offset + Math::vec2{0, m_texture_extent.y}},
		{points[2], m_texture_offset + m_texture_extent}
	}};
}

void Image::record_render_commands(RenderCommands& rc)
{
	rc.texture(m_buffer_offset + Element::c_buffer_size, 4, m_texture->set_index);
}

void Image::arrange()
{
	auto w = m_texture->w, h = m_texture->h;

	auto th = m_position.w * h / w;

	if(m_position.h < th) // Image too "tall"
	{
		auto w_red = m_position.w - m_position.h * w / h;
		assert(m_position.w >= m_position.h * w / h);
		m_position.x += w_red / 2;
		m_position.w -= w_red;
	}
	else
	{
		auto h_red = m_position.h - th;
		m_position.y += h_red / 2;
		m_position.h -= h_red;
	}
}

void Label::init()
{
  Element::init();
  if(m_font == nullptr)
    m_font = base()->default_font();
}

void Label::upload_buffer(char* buffer_data_ptr)
{
  m_n_glyphs = text_write_buffer(buffer_data_ptr, m_font->glyphs, m_text, m_text_orig);
}

void Label::arrange()
{
  Rect bbox = m_font->glyphs.compute_text_size(m_text);

  m_text_orig = 
  {
    position().x + position().w / 2 - bbox.w / 2, 
    position().y + position().h / 2 - bbox.h / 2
  };

  m_text_orig -= Math::vec2{bbox.x, bbox.y};
}

void Label::record_render_commands(RenderCommands& rc)
{
  text_render(rc, m_buffer_offset, m_n_glyphs, m_font->set_index);
}

Extent Label::get_min_dimensions()
{
  return m_font->glyphs.compute_text_size(m_text).extent();
}

void Grid::init()
{
  Element::init();
  for(auto& elem : m_grid_elements)
    elem.elem->init();
}

Extent Grid::get_min_dimensions() 
{
  if(!min_dimensions_computed())
    compute_min_dimensions();

  m_min_width = std::accumulate(m_min_col_width.begin(), m_min_col_width.end(), 0.0f);
  m_min_height = std::accumulate(m_min_row_height.begin(), m_min_row_height.end(), 0.0f);

  return {
    m_min_width,
    m_min_height
  };
}

void Grid::arrange()
{
  if(!min_dimensions_computed())
    compute_min_dimensions();

  px_t exceed_w_per_col = (position().w - m_min_width) / float(m_min_col_width.size());
  px_t exceed_h_per_row = (position().h - m_min_height) / float(m_min_row_height.size());

  std::vector<px_t> row_x(m_min_col_width.size()), col_y(m_min_row_height.size());

  std::exclusive_scan(m_min_col_width.begin(), m_min_col_width.end(), row_x.begin(), position().x,
    [exceed_w_per_col](auto a, auto b) {return a + b + exceed_w_per_col;}
  );

  std::exclusive_scan(m_min_row_height.begin(), m_min_row_height.end(), col_y.begin(), position().y,
    [exceed_h_per_row](auto a, auto b) {return a + b + exceed_h_per_row;}
  );

  for(auto& elem : m_grid_elements)
  {
    auto x = row_x[elem.row], y = col_y[elem.col];
    elem.elem->set_position({
      x,
      y,
      m_min_col_width[elem.row] + exceed_w_per_col,
      m_min_row_height[elem.row] + exceed_h_per_row
    });

    elem.elem->arrange();
  }
}

void Grid::record_render_commands(RenderCommands& rc)
{
  for(auto& elem : m_grid_elements)
    elem.elem->record_render_commands(rc);
}

void Grid::compute_min_dimensions()
{
  m_min_col_width.clear();
  m_min_row_height.clear();

  for(auto& elem : m_grid_elements)
  {
    auto [w, h] = elem.elem->get_min_dimensions();

    if(m_min_col_width.size() <= elem.col)
      m_min_col_width.resize(elem.col + 1);
    if(m_min_row_height.size() <= elem.row)
      m_min_row_height.resize(elem.row + 1);

    m_min_col_width[elem.col] = std::min(m_min_col_width[elem.col], w);
    m_min_row_height[elem.row] = std::min(m_min_row_height[elem.row], h);
  }
}

void FramedElement::init()
{
  Frame::init();
  m_contained_element->init();
}

Extent FramedElement::get_min_dimensions()
{
  Extent inner = m_contained_element->get_min_dimensions();
  inner.w += 2 * border();
  inner.h += 2 * margin();
  
  return inner;
}

void FramedElement::record_render_commands(RenderCommands& rc)
{
  Frame::record_render_commands(rc);
  m_contained_element->record_render_commands(rc);
}

void FramedElement::arrange()
{
  auto bord_marg = border() + margin();
  m_contained_element->set_position({
    position().x + bord_marg,
    position().y + bord_marg,
    position().w - 2.0f * bord_marg,
    position().h - 2.0f * bord_marg
  });
  m_contained_element->arrange();
}
