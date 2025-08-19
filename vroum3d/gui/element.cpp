#include "element.h"
#include "../core/vkutil.h"

using namespace Vroum3d::Gui;

VkDeviceSize Menu::get_buffer_size() const
{
	VkDeviceSize size(0);
	for(auto& elem : m_children)
		size += elem->get_buffer_size();

	return size;
}

void Menu::set_buffer_offset(VkDeviceSize offset)
{
	for(auto& elem : m_children)
	{
		elem->set_buffer_offset(offset);
		offset += elem->get_buffer_size();
	}
}


