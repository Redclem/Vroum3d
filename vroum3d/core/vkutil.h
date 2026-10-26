#ifndef VROUMD3D_CORE_VKUTIL_H_INCLUDED
#define VROUMD3D_CORE_VKUTIL_H_INCLUDED

#include "../volk_wrapper.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace Vroum3d::vkutil
{

constexpr VkComponentMapping components_id{VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

constexpr VkImageSubresourceRange color_subres_plain = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

inline VkImageViewType view_type_from_image_type(VkImageType type)
{
  switch(type)
  {
  case VK_IMAGE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
  case VK_IMAGE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
  case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
  default: throw std::runtime_error("Unhandled image type value");
  }
}

std::uint32_t find_mem_index(VkPhysicalDevice pdev, const VkMemoryRequirements &mr, VkMemoryPropertyFlags mp);

constexpr void add_mem_reqs(VkMemoryRequirements &a, const VkMemoryRequirements &b)
{
  a.memoryTypeBits &= b.memoryTypeBits;
  a.alignment = std::max(a.alignment, b.alignment);
  if(auto mod = a.size / b.alignment; mod) a.size += (b.alignment - mod);
  a.size += b.size;
}

constexpr VkDeviceSize match_offset(VkDeviceSize offset, VkDeviceSize alignment)
{
  if(auto mod = offset % alignment; mod) return offset + alignment - mod;
  return offset;
}

} // namespace Vroum3d::vkutil


#endif /* end of include guard */
