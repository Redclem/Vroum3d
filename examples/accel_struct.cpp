#include "../vroum3d/vroum3d.h"
#include "raytracing/acceleration_structure.h"
#include "raytracing/instance_info.h"

#include <cstring>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Core;
using namespace Vroum3d;

int main(int, char*[])
{
  Instance inst(Raytracing::InstanceInfo<>{});

  VkCommandPool cmd_pool;
  {
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, inst.compute_queue_index()};

    vk_check(vkCreateCommandPool(inst.device(), &ci, nullptr, &cmd_pool));
  }

  Math::vec3 data[] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

  uint32_t indexes[] = {0, 1, 2};

  Buffer buf(inst, sizeof(data) + sizeof(indexes), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  auto* dt = buf.map();
  memcpy(dt, data, sizeof(data));
  memcpy(dt + sizeof(data), indexes, sizeof(indexes));

  buf.flush_unmap();

  Raytracing::SimpleTriangleAccelerationStructure as(inst);

  VkDeviceSize scratch_size;
  as.create_as(3, 3, scratch_size);

  {
    Buffer scratch_buffer(inst, scratch_size,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 256);

    auto addr = buf.device_address();

    as.upload_as({.deviceAddress = addr}, {.deviceAddress = addr + sizeof(data)}, cmd_pool, 3, 3, scratch_buffer.buffer(),
                 {.deviceAddress = scratch_buffer.device_address()});
  }

  vkDestroyCommandPool(inst.device(), cmd_pool, nullptr);

  return 0;
}
