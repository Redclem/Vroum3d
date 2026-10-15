#include "acceleration_structure.h"
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Raytracing;

void SimpleTriangleAccelerationStructure::destroy()
{
  m_blas.destroy_with([&](VkAccelerationStructureKHR as){
    vkDestroyAccelerationStructureKHR(device(), as, nullptr);
      });
  m_tlas.destroy_with([&](VkAccelerationStructureKHR as){
    vkDestroyAccelerationStructureKHR(device(), as, nullptr);
      });

  m_buffer.destroy_with([&](auto buf){vkDestroyBuffer(device(), buf, nullptr);});
  m_instance->free(m_buffer_mem);
}

void SimpleTriangleAccelerationStructure::create_as(VkBuffer data_source, uint32_t n_vertex, uint32_t n_index)
{
  VkBufferDeviceAddressInfo bdai{
    VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    nullptr,
    data_source
  };

  //VkDeviceAddress input_buffer_address = vkGetBufferDeviceAddress(device(), &bdai);

  VkAccelerationStructureGeometryKHR bl_geom{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    nullptr,
    VK_GEOMETRY_TYPE_TRIANGLES_KHR,
    {
      .triangles = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        nullptr,
        VK_FORMAT_R32G32B32_SFLOAT,
        {},
        sizeof(vertex_t),
        n_vertex - 1,
        VK_INDEX_TYPE_UINT32,
        {},
        {.hostAddress = nullptr}
      }
    },
    0
  };

  VkAccelerationStructureBuildGeometryInfoKHR bl_bgi{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      nullptr,
      VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      0,
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      VK_NULL_HANDLE,
      VK_NULL_HANDLE,
      1,
      &bl_geom,
      nullptr,
      {}
  };
  
  VkAccelerationStructureInstanceKHR bl_instance{
    VkTransformMatrixKHR(),
    0,
    0xFF,
    0,
    0xFF,
    0
  };

  VkAccelerationStructureGeometryKHR tl_geom{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    nullptr,
    VK_GEOMETRY_TYPE_INSTANCES_KHR,
    {
      .instances = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        nullptr,
        VK_FALSE,
        {.hostAddress = &bl_instance}
      }
    },
    0
  };

  VkAccelerationStructureBuildGeometryInfoKHR tl_bgi{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      nullptr,
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      0,
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      VK_NULL_HANDLE,
      VK_NULL_HANDLE,
      1,
      &tl_geom,
      nullptr,
      {}
  };

  VkAccelerationStructureBuildSizesInfoKHR bl_size_info, tl_size_info;

  uint32_t prim_count = n_index / 3, one = 1;

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bl_bgi, &prim_count, &bl_size_info);

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tl_bgi, &one, &tl_size_info);

  create_buffer(bl_size_info.accelerationStructureSize + tl_size_info.accelerationStructureSize);

  bdai.buffer = m_buffer;

  VkDeviceAddress as_buffer_address = vkGetBufferDeviceAddress(device(), &bdai);

  VkAccelerationStructureCreateInfo2KHR asci{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    nullptr,
    0,
    {as_buffer_address, bl_size_info.accelerationStructureSize},
    0,
    VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
  };

  vk_check(vkCreateAccelerationStructure2KHR(device(), &asci, nullptr, &m_blas));

  asci.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  asci.addressRange = {as_buffer_address + bl_size_info.accelerationStructureSize, tl_size_info.accelerationStructureSize};

  vk_check(vkCreateAccelerationStructure2KHR(device(), &asci, nullptr, &m_tlas));
}

void SimpleTriangleAccelerationStructure::create_buffer(VkDeviceSize size)
{
  VkBufferCreateInfo bi
  {
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      size,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr
  };

  vk_check(vkCreateBuffer(device(), &bi, nullptr, &m_buffer));

  VkMemoryRequirements mr;
  vkGetBufferMemoryRequirements(device(), m_buffer, &mr);

  m_buffer_mem = m_instance->allocate(mr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkBindBufferMemory(device(), m_buffer, m_buffer_mem.memory(), m_buffer_mem.offset());
}
