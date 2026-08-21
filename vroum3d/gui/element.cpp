#include "element.h"
#include "../core/vkutil.h"
#include "base.h"
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

void Frame::upload_buffer()
{
	RenderData& rd = *reinterpret_cast<RenderData*>(m_buffer_data_ptr + Element::c_buffer_size);

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
}

void Frame::record_render_commands(RenderCommands& rc)
{
	rc.fills.emplace_back(m_buffer_offset + Element::c_buffer_size, 8);
}
