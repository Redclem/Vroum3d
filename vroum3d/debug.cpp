#include "debug.h"

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

const char* Vroum3d::Debug::String::VkResult(enum VkResult res)
{
  return string_VkResult(res);
}

std::string Vroum3d::Debug::String::VkDebugUtilsMessageTypeFlagsEXT(::VkDebugUtilsMessageTypeFlagsEXT flags)
{
  return string_VkDebugUtilsMessageTypeFlagsEXT(flags);
}

std::string Vroum3d::Debug::String::VkDebugUtilsMessageSeverityFlagsEXT(::VkDebugUtilsMessageSeverityFlagsEXT flags)
{
  return string_VkDebugUtilsMessageSeverityFlagsEXT(flags);
}
