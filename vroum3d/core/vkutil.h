#ifndef VROUMD3D_CORE_VKUTIL_H_INCLUDED
#define VROUMD3D_CORE_VKUTIL_H_INCLUDED

#include <algorithm>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace Vroum3d::vkutil{

constexpr VkComponentMapping components_id{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};

constexpr VkImageSubresourceRange color_subres_plain = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

std::uint32_t find_mem_index(VkPhysicalDevice pdev, const VkMemoryRequirements& mr, VkMemoryPropertyFlags mp);

constexpr void add_mem_reqs(VkMemoryRequirements& a, const VkMemoryRequirements& b)
{
	a.memoryTypeBits &= b.memoryTypeBits;
	a.alignment = std::max(a.alignment, b.alignment);
	if(auto mod = a.size / b.alignment; mod)
		a.size += (b.alignment - mod);
	a.size += b.size;
}

constexpr VkDeviceSize match_offset(VkDeviceSize a, VkDeviceSize b)
{
	if(auto mod = a % b; mod) return a + b - mod;
	return a;
}

}


#endif /* end of include guard */
