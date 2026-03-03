#include "element.h"
#include "../core/vkutil.h"
#include "base.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Gui;

VkDeviceSize Menu::get_buffer_size() const
{
	return 0;
}

void Element::init()
{
	m_base->register_element(this);
}

void Element::record_upl_commands(VkCommandBuffer)
{
}


