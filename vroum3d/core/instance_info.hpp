#ifndef VROUM3D_CORE_INSTANCE_INFO_HPP_INCLUDED
#define VROUM3D_CORE_INSTANCE_INFO_HPP_INCLUDED

#include "../debug.h"
#include "../volk_wrapper.h"
#include "display.hpp"

#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

template<bool debug_enable>
struct InstanceInfoDebug
{
  void* instance_pnext() const { return nullptr; }

  VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_info() const { return nullptr; }
};

template<>
struct InstanceInfoDebug<true>
{
  static constexpr VkBool32 c_sync_val_enabled = VK_TRUE;
  static constexpr std::array c_layer_settings = []() {
    if constexpr(c_enable_syncval)
      return std::array<VkLayerSettingEXT, 1>{{{"VK_LAYER_KHRONOS_validation", "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &c_sync_val_enabled}}};
    else
      return std::array<VkLayerSettingEXT, 0>();
  }();

  VkLayerSettingsCreateInfoEXT lays_s = {VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, c_layer_settings.size(), c_layer_settings.data()};

  static VkBool32 callback(VkDebugUtilsMessageSeverityFlagBitsEXT sever, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* cbd, void*)
  {
    Vroum3d::log("Vulkan Message:");
    Vroum3d::log("Severity:", Debug::String::VkDebugUtilsMessageSeverityFlagsEXT(sever));
    Vroum3d::log("Type:", Debug::String::VkDebugUtilsMessageTypeFlagsEXT(type));
    Vroum3d::log(cbd->pMessage, "\n\n");
    return VK_FALSE;
  }

  VkDebugUtilsMessengerCreateInfoEXT dbi = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                                            nullptr,
                                            0,
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                                            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                              | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                                            callback,
                                            nullptr};

  InstanceInfoDebug() {}

  void* instance_pnext()
  {
    if constexpr(c_layer_settings.size() > 0)
      dbi.pNext = &lays_s;
    return reinterpret_cast<void*>(&dbi);
  }

  const VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_info() { return &dbi; }
};

class DefaultInstanceInfo : public InstanceInfoDebug<debug> {
  /* Base structure for InstanceInfos, i.e. structs to guide the constructor of an Instance about choices regarding the instance and device creation parameters.
   * Includes but not restricted to instance layers and extensions and device extensions.
   *
   * It can be assumed the InstanceInfo will not be destroyed nor moved during the execution of the instance constructor.
   * This means you can pass pNext chains using members of the InstanceInfo.
   *
   * The following will happen between the Instance being created and the InstanceInfo:
   *
   * -> VkInstance created, with extensions and layers returned from inst_exts_lays() and pNext from instance_pnext().
   * -> (if debug) debug messenger created, with debug messenger creation info from debug_messenger_info().
   * -> (if display) send VkSurfaceKHR handle to InstanceInfo using set_surface
   * -> Physical Devices rated by int64_t rate_physical_device(VkPhysicalDevice, void*)
   * -> Physical Device features queried by query_features(VkPhysicalDevice) and store in struct (Not in vkXXfeats, they may be cleared!)
   * -> device pNext chain built and returned by build_device_pnext(VkPhysicalDevice) using structs in InstanceInfo
   * -> device create with extensions from dev_exts()
   *
   * */


protected:
  VkPhysicalDeviceVulkan13Features m_Vulkan13Features{};
  VkPhysicalDeviceVulkan12Features m_Vulkan12Features{};
  VkPhysicalDeviceFeatures2 m_Features2;
  VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};

  bool m_dynamicRendering : 1, m_synchronization2 : 1, m_runtimeDescriptorArray : 1;

private:
  void* m_device_pnext = nullptr;
  std::vector<std::string> m_extensions;

public:
  struct ExtensionsLayers
  {
    std::vector<std::string> exts, lays;
  };

  VkBool32 enable_device_address() const { return m_Vulkan12Features.bufferDeviceAddress; }

  DefaultInstanceInfo()
  {
    m_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    m_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  }

  template<bool dbg = false>
  struct DefaultInstanceExtensions
  {
    static constexpr std::array value = []() {
      if constexpr(dbg && c_enable_syncval)
        return std::array<const char*, 1>{"VK_EXT_layer_settings"};
      else
        return std::array<const char*, 0>{};
    }();
  };

  template<bool dbg = false>
  struct DefaultInstanceLayers
  {
    static constexpr std::array value = []() {
      if constexpr(dbg)
        return std::array<const char*, 1>{"VK_LAYER_KHRONOS_validation"};
      else
        return std::array<const char*, 0>{};
    }();
  };

  ExtensionsLayers inst_exts_lays()
  {
    constexpr auto extarray = DefaultInstanceExtensions<debug>::value;
    constexpr auto layarray = DefaultInstanceLayers<debug>::value;

    return {{extarray.begin(), extarray.end()}, {layarray.begin(), layarray.end()}};
  }

  void build_device_pnext()
  {
    m_Vulkan12Features = {};
    m_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    m_Vulkan13Features = {};
    m_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    m_device_pnext = nullptr;

    add_device_pnext(&m_Vulkan12Features);
    add_device_pnext(&m_Vulkan13Features);

    if(m_synchronization2)
      m_Vulkan13Features.synchronization2 = VK_TRUE;
    if(m_dynamicRendering)
      m_Vulkan13Features.dynamicRendering = VK_TRUE;
    if(m_runtimeDescriptorArray)
      m_Vulkan12Features.runtimeDescriptorArray = VK_TRUE;
  }

  std::vector<std::string> &dev_exts() { return m_extensions; }

  void* device_pnext() { return m_device_pnext; }

  using InstanceInfoDebug<debug>::instance_pnext;

  template<typename NewStr>
  static void check_add_string(std::vector<std::string> &strings, NewStr &&str)
  {
    if(std::find(strings.begin(), strings.end(), str) == strings.end())
      strings.push_back(std::forward<NewStr>(str));
  }

  /** Add structure to pNext chain of device create info.
   * Ensure the added struct will live for at least the same time as the
   * instance info.
   */
  template<typename Struct>
  void add_device_pnext(Struct* s)
  {
    s->pNext = m_device_pnext;
    m_device_pnext = s;
  }

  /** Rate physical device (according to criteria such as performance, extensions supported, ...).
   * Elements of this structure may be used to query device features.
   * A negative return value means the device is not suitable for use.
   * Default function gives zero to all devices.
   */
  int64_t rate_physical_device(VkPhysicalDevice pdev, void* pNext = nullptr)
  {
    m_Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_Features2.pNext = &m_Vulkan12Features;
    m_Vulkan12Features.pNext = &m_Vulkan13Features;
    m_Vulkan13Features.pNext = pNext;

    vkGetPhysicalDeviceFeatures2(pdev, &m_Features2);

    auto dev_exts = wrap_enumerate(vkEnumerateDeviceExtensionProperties, pdev, nullptr);

    for(auto &s: m_extensions)
    {
      if(!std::any_of(dev_exts.begin(), dev_exts.end(), [&](const auto &ext) { return s == ext.extensionName; }))
        return -1;
    }

    vkGetPhysicalDeviceProperties(pdev, &m_PhysicalDeviceProperties);

    int score = 0;

    switch(m_PhysicalDeviceProperties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score = 2; break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score = 1; break;
    default: break;
    }

    return score;
  }

  /** Query physical device features in member variables */
  void query_features(VkPhysicalDevice pdev)
  {
    m_Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_Features2.pNext = &m_Vulkan12Features;
    m_Vulkan12Features.pNext = &m_Vulkan13Features;

    vkGetPhysicalDeviceFeatures2(pdev, &m_Features2);

    m_dynamicRendering = m_Vulkan13Features.dynamicRendering == VK_TRUE;
    m_synchronization2 = m_Vulkan13Features.synchronization2 == VK_TRUE;

    m_runtimeDescriptorArray = m_Vulkan12Features.runtimeDescriptorArray == VK_TRUE;
  }

  /**
   * Add required extension.
   *
   * Add an extension to be required for any physical device. Checks for presence of the extension will be handled automatically by the DefaultInstanceInfo class when
   * rating. The extension will also be present in the returned dev_exts().
   *
   */
  template<typename String>
  void add_required_extension(String &&s)
  {
    check_add_string(m_extensions, std::forward<String>(s));
  }
};

template<typename InstanceInfo = DefaultInstanceInfo>
struct DisplayInstanceInfo : public InstanceInfo
{
  using ExtensionsLayers = DefaultInstanceInfo::ExtensionsLayers;

  VkSurfaceKHR m_surf;

  template<typename... Args>
  DisplayInstanceInfo(Args &&... args) : InstanceInfo(std::forward<Args>(args)...)
  {}

  void set_surface(VkSurfaceKHR surf) { m_surf = surf; }

  ExtensionsLayers inst_exts_lays()
  {
    auto el = InstanceInfo::inst_exts_lays();

    Uint32 n_exts;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&n_exts);

    for(auto elem = exts, end = exts + n_exts; elem != end; ++elem)
    {
      if(!std::any_of(el.exts.begin(), el.exts.end(), [elem](const std::string &s) { return s == *elem; }))
        el.exts.emplace_back(*elem);
    }
    return el;
  }

  auto &dev_exts()
  {
    auto &vec = InstanceInfo::dev_exts();
    vec.push_back("VK_KHR_swapchain");
    return vec;
  }

  VkImageUsageFlags depth_image_additional_usage() const { return 0; }

  int64_t rate_physical_device(VkPhysicalDevice pdev, void* pNext = nullptr)
  {
    auto score = InstanceInfo::rate_physical_device(pdev, pNext);

    if(this->m_Vulkan13Features.dynamicRendering != VK_TRUE)
      return -1;

    uint32_t n_q_props;
    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &n_q_props, nullptr);

    VkBool32 supp(false);

    for(uint32_t queue_index(0); queue_index != n_q_props; queue_index++)
    {
      vkGetPhysicalDeviceSurfaceSupportKHR(pdev, queue_index, m_surf, &supp);
      if(supp == VK_TRUE)
        break;
    }

    if(supp != VK_TRUE)
      return -1;

    return score;
  }
};


} // namespace Vroum3d::Core

#endif
