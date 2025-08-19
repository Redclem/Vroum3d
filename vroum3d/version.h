#ifndef VROUM3D_VERSION_H_INCLUDED
#define VROUM3D_VERSION_H_INCLUDED

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Version
{
	constexpr std::uint32_t major = 0;
	constexpr std::uint32_t minor = 1;
	constexpr std::uint32_t patch = 0;

	constexpr std::uint32_t eng_ver = VK_MAKE_VERSION(major, minor, patch);

	constexpr std::uint32_t vk_api_ver = VK_API_VERSION_1_4;
}

#endif
