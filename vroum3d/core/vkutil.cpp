#include "vkutil.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::vkutil
{

std::uint32_t find_mem_index(VkPhysicalDevice pdev, const VkMemoryRequirements& mr, VkMemoryPropertyFlags mp)
{
	VkPhysicalDeviceMemoryProperties pdmp;
	vkGetPhysicalDeviceMemoryProperties(pdev, &pdmp);

	std::uint32_t bit(1);
	for(std::uint32_t idx = 0; idx != pdmp.memoryTypeCount; idx++, bit <<= 1)
		if(bit & mr.memoryTypeBits && (mp & pdmp.memoryTypes[idx].propertyFlags) == mp)
			return idx;

	throw std::runtime_error("No matching memory type");
}

}
