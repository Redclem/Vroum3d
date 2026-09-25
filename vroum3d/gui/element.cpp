#include "element.h"
#include "../core/vkutil.h"
#include "base.h"
#include "common.h"
#include <cassert>
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

void Frame::upload_buffer(char * buffer_data_ptr )
{
	RenderData& rd = *reinterpret_cast<RenderData*>(buffer_data_ptr + Element::c_buffer_size + m_buffer_offset);

	Rect r(position());
	r.shrink(m_margin);
	auto pts_outer = r.rect_points();
	r.shrink(m_border);
	auto pts_inner = r.rect_points();

	rd.points[0] = pts_outer[0];
	rd.points[1] = pts_inner[0];
	rd.points[2] = pts_outer[1];
	rd.points[3] = pts_inner[1];
	rd.points[4] = pts_outer[2];
	rd.points[5] = pts_inner[2];
	rd.points[6] = pts_outer[3];
	rd.points[7] = pts_inner[3];
	rd.points[8] = pts_outer[0];
	rd.points[9] = pts_inner[0];
}

void Frame::record_render_commands(RenderCommands& rc)
{
	rc.fill(m_buffer_offset + Element::c_buffer_size, 10);
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
  auto* glyph_out_iter(reinterpret_cast<GlyphData*>(buffer_data_ptr));

  float adv(0.0f);

  auto proc_point = [&](char32_t point)
    {
      auto iter = m_font->glyphs.find(point);
      if(iter == m_font->glyphs.end()) return;

      const auto& g = iter->second;

      glyph_out_iter->pts[0] = {{m_text_orig.x + g.offset.x, g.offset.y + m_text_orig.y}, g.start};
      glyph_out_iter->pts[1] = {{m_text_orig.x + g.offset.x + g.w, g.offset.y + m_text_orig.y}, {g.end.x, g.start.y}};
      glyph_out_iter->pts[2] = {{m_text_orig.x + g.offset.x, g.offset.y + g.h + m_text_orig.y}, {g.start.x, g.end.y}};
      glyph_out_iter->pts[3] = {{m_text_orig.x + g.offset.x + g.w, g.offset.y + g.h + m_text_orig.y}, {g.start.x, g.start.y}};

      glyph_out_iter++;
      adv += g.adv;
    };

  m_font->glyphs.iterate_unicode_points(m_text, proc_point);
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

}
