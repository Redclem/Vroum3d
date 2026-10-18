#ifndef VROUM3D_RAYTRACING_INSTANCE_INFO_H
#define VROUM3D_RAYTRACING_INSTANCE_INFO_H

#include "../core/instance.h"

#include <vulkan/vulkan_core.h>

namespace Vroum3d::Raytracing
{

template<typename BaseInstanceInfo = Core::DefaultInstanceInfo>
class InstanceInfo : public BaseInstanceInfo {
protected:
  VkPhysicalDeviceAccelerationStructureFeaturesKHR m_AccelerationStructureFeaturesKHR{};
  VkPhysicalDeviceRayQueryFeaturesKHR m_RayQueryFeaturesKHR{};

public:
  template<typename... Args>
  InstanceInfo(Args &&... ags) : BaseInstanceInfo(std::forward<Args>(ags)...)
  {
    m_AccelerationStructureFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    m_RayQueryFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;


    Core::DefaultInstanceInfo::add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    Core::DefaultInstanceInfo::add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    Core::DefaultInstanceInfo::add_required_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
  }

  int64_t rate_physical_device(VkPhysicalDevice pdev, void* pNext = nullptr)
  {
    m_AccelerationStructureFeaturesKHR.pNext = &m_RayQueryFeaturesKHR;
    m_RayQueryFeaturesKHR.pNext = pNext;
    int64_t score = BaseInstanceInfo::rate_physical_device(pdev, &m_AccelerationStructureFeaturesKHR);

    if(score < 0)
      return score;

    if(m_AccelerationStructureFeaturesKHR.accelerationStructure != VK_TRUE)
      return -1;
    if(m_RayQueryFeaturesKHR.rayQuery != VK_TRUE)
      return -1;

    return score;
  }

  void build_device_pnext()
  {
    BaseInstanceInfo::build_device_pnext();

    m_AccelerationStructureFeaturesKHR.accelerationStructure = VK_TRUE;
    m_RayQueryFeaturesKHR.rayQuery = VK_TRUE;

    this->m_Vulkan12Features.bufferDeviceAddress = VK_TRUE;
    BaseInstanceInfo::add_device_pnext(&m_AccelerationStructureFeaturesKHR);
    BaseInstanceInfo::add_device_pnext(&m_RayQueryFeaturesKHR);
  }

  std::vector<std::string> dev_exts()
  {
    auto exts = BaseInstanceInfo::dev_exts();

    return exts;
  }
};

} // namespace Vroum3d::Raytracing

#endif
