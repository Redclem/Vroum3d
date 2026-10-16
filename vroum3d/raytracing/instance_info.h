#ifndef VROUM3D_RAYTRACING_INSTANCE_INFO_H
#define VROUM3D_RAYTRACING_INSTANCE_INFO_H

#include "../core/instance.h"
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Raytracing
{

template<typename BaseInstanceInfo = Core::DefaultInstanceInfo>
class InstanceInfo : public BaseInstanceInfo
{
protected:
  VkPhysicalDeviceAccelerationStructureFeaturesKHR acc_struct_feats{};
public:
  template<typename ... Args>
  InstanceInfo(Args&& ... ags) : BaseInstanceInfo(std::forward<Args>(ags)...)
  {
    acc_struct_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acc_struct_feats.accelerationStructure = VK_TRUE;

    this->vk12feats.bufferDeviceAddress = VK_TRUE;
  }

  void build_device_pnext_chain()
  {
    BaseInstanceInfo::build_device_pnext_chain();

    BaseInstanceInfo::add_device_pnext(&acc_struct_feats);
  }

  std::vector<std::string> dev_exts()
  {
    auto exts = BaseInstanceInfo::dev_exts();

    Core::DefaultInstanceInfo::check_add_string(exts, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    Core::DefaultInstanceInfo::check_add_string(exts, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    return exts;
  }


};

}

#endif
