#include "acceleration_structure.h"

#include "../core/objects.h"

#include <algorithm>
#include <vulkan/vulkan_core.h>

using namespace Vroum3d::Raytracing;

void SimpleTriangleAccelerationStructure::destroy()
{
  m_blas.destroy_with([&](VkAccelerationStructureKHR as) { vkDestroyAccelerationStructureKHR(device(), as, nullptr); });
  m_tlas.destroy_with([&](VkAccelerationStructureKHR as) { vkDestroyAccelerationStructureKHR(device(), as, nullptr); });

  m_buffer.destroy_with([&](auto buf) { vkDestroyBuffer(device(), buf, nullptr); });
  m_instance->free(m_buffer_mem);
}

void SimpleTriangleAccelerationStructure::create_as(uint32_t n_vertex, uint32_t n_index, VkDeviceSize &scratch_size)
{
  VkAccelerationStructureGeometryKHR bl_geom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                             nullptr,
                                             VK_GEOMETRY_TYPE_TRIANGLES_KHR,
                                             {.triangles = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                                                            nullptr,
                                                            VK_FORMAT_R32G32B32_SFLOAT,
                                                            {},
                                                            sizeof(vertex_t),
                                                            n_vertex - 1,
                                                            VK_INDEX_TYPE_UINT32,
                                                            {},
                                                            {.hostAddress = nullptr}}},
                                             0};

  VkAccelerationStructureBuildGeometryInfoKHR bl_bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                     nullptr,
                                                     VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                     0,
                                                     VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                     VK_NULL_HANDLE,
                                                     VK_NULL_HANDLE,
                                                     1,
                                                     &bl_geom,
                                                     nullptr,
                                                     {}};

  VkAccelerationStructureGeometryKHR tl_geom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                             nullptr,
                                             VK_GEOMETRY_TYPE_INSTANCES_KHR,
                                             {.instances = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, nullptr, VK_FALSE, {}}},
                                             0};

  VkAccelerationStructureBuildGeometryInfoKHR tl_bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                     nullptr,
                                                     VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                                                     0,
                                                     VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                     VK_NULL_HANDLE,
                                                     VK_NULL_HANDLE,
                                                     1,
                                                     &tl_geom,
                                                     nullptr,
                                                     {}};

  VkAccelerationStructureBuildSizesInfoKHR bl_size_info, tl_size_info;
  bl_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  bl_size_info.pNext = nullptr;
  tl_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  tl_size_info.pNext = nullptr;

  uint32_t prim_count = n_index / 3, one = 1;

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bl_bgi, &prim_count, &bl_size_info);

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tl_bgi, &one, &tl_size_info);

  if(bl_size_info.accelerationStructureSize & 255)
    bl_size_info.accelerationStructureSize = (bl_size_info.accelerationStructureSize & ~255) + 256;

  create_buffer(bl_size_info.accelerationStructureSize + tl_size_info.accelerationStructureSize);

  // bdai.buffer = m_buffer;

  VkAccelerationStructureCreateInfoKHR asci{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, nullptr, 0, m_buffer, 0, bl_size_info.accelerationStructureSize,
                                            VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,          0};

  vk_check(vkCreateAccelerationStructureKHR(device(), &asci, nullptr, &m_blas));

  asci.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  asci.offset = asci.size;
  asci.size = tl_size_info.accelerationStructureSize;


  vk_check(vkCreateAccelerationStructureKHR(device(), &asci, nullptr, &m_tlas));

  scratch_size = std::max(bl_size_info.buildScratchSize, tl_size_info.buildScratchSize);
}

void SimpleTriangleAccelerationStructure::create_buffer(VkDeviceSize size)
{
  VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        nullptr,
                        0,
                        size,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0,
                        nullptr};

  vk_check(vkCreateBuffer(device(), &bi, nullptr, &m_buffer));

  m_buffer_mem = m_instance->allocate_bind(m_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SimpleTriangleAccelerationStructure::upload_as(VkDeviceOrHostAddressConstKHR vertex_data, VkDeviceOrHostAddressConstKHR index_data, VkCommandPool cmd_pool,
                                                    uint32_t n_vertex, uint32_t n_index, VkBuffer scratch_buffer, VkDeviceOrHostAddressKHR scratch_addr)
{
  /*VkBufferDeviceAddressInfo bdi{
    VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    nullptr,
    m_buffer
  };

  VkDeviceAddress adr = vkGetBufferDeviceAddress(device(), &bdi);*/

  Core::CommandBuffer cmd_buf(*m_instance, cmd_pool);
  cmd_buf.begin_primary();

  VkAccelerationStructureGeometryKHR geom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                          nullptr,
                                          VK_GEOMETRY_TYPE_TRIANGLES_KHR,
                                          {.triangles = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                                                         nullptr,
                                                         VK_FORMAT_R32G32B32_SFLOAT,
                                                         vertex_data,
                                                         sizeof(vertex_t),
                                                         n_vertex - 1,
                                                         VK_INDEX_TYPE_UINT32,
                                                         index_data,
                                                         {.hostAddress = nullptr}}},
                                          0 & VK_GEOMETRY_OPAQUE_BIT_KHR};

  VkAccelerationStructureBuildGeometryInfoKHR bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                  nullptr,
                                                  VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                  0,
                                                  VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                  VK_NULL_HANDLE,
                                                  m_blas,
                                                  1,
                                                  &geom,
                                                  nullptr,
                                                  scratch_addr};

  VkAccelerationStructureBuildRangeInfoKHR asbr{n_index / 3, 0, 0, 0};

  auto p_asbr = &asbr;

  cmd_buf.cmd(vkCmdBuildAccelerationStructuresKHR, 1, &bgi, &p_asbr);

  VkBufferMemoryBarrier2 bmb{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                             nullptr,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                             0,
                             0,
                             scratch_buffer,
                             0,
                             VK_WHOLE_SIZE};

  VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO, nullptr, 0, 0, nullptr, 1, &bmb, 0, nullptr};

  cmd_buf.cmd(vkCmdPipelineBarrier2, &di);

  VkAccelerationStructureDeviceAddressInfoKHR asdai{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, nullptr, m_blas};

  VkDeviceAddress blas_address = vkGetAccelerationStructureDeviceAddressKHR(device(), &asdai);

  Core::Buffer tlas_build_buffer(*m_instance, sizeof(VkAccelerationStructureInstanceKHR),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  *reinterpret_cast<VkAccelerationStructureInstanceKHR*>(tlas_build_buffer.map()) = {
    VkTransformMatrixKHR{{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}}, 0, 0xFF, 0, 0, blas_address};

  tlas_build_buffer.flush_unmap();

  geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geom.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, nullptr, VK_FALSE, {.deviceAddress = tlas_build_buffer.device_address()}};

  bgi.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  bgi.dstAccelerationStructure = m_tlas;

  asbr.primitiveCount = 1;

  cmd_buf.cmd(vkCmdBuildAccelerationStructuresKHR, 1, &bgi, &p_asbr);

  cmd_buf.end();
  m_instance->quick_submit(cmd_buf.cmd_buf(), m_instance->compute_queue());
}

void SimpleTriangleAccelerationStructure::create_as(const std::vector<Mesh> &meshes, VkDeviceSize &scratch_size)
{
  std::vector<VkAccelerationStructureGeometryKHR> bl_geoms(meshes.size());

  std::transform(meshes.begin(), meshes.end(), bl_geoms.begin(), [&](const Mesh &mesh) -> VkAccelerationStructureGeometryKHR {
    return {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            nullptr,
            VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            {.triangles = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                           nullptr,
                           VK_FORMAT_R32G32B32_SFLOAT,
                           {},
                           sizeof(vertex_t),
                           mesh.n_vertex - 1,
                           VK_INDEX_TYPE_UINT32,
                           {},
                           {.hostAddress = nullptr}}},
            0};
  });

  VkAccelerationStructureBuildGeometryInfoKHR bl_bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                     nullptr,
                                                     VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                     0,
                                                     VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                     VK_NULL_HANDLE,
                                                     VK_NULL_HANDLE,
                                                     uint32_t(bl_geoms.size()),
                                                     bl_geoms.data(),
                                                     nullptr,
                                                     {}};

  VkAccelerationStructureGeometryKHR tl_geom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                             nullptr,
                                             VK_GEOMETRY_TYPE_INSTANCES_KHR,
                                             {.instances = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, nullptr, VK_FALSE, {}}},
                                             0};

  VkAccelerationStructureBuildGeometryInfoKHR tl_bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                     nullptr,
                                                     VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                                                     0,
                                                     VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                     VK_NULL_HANDLE,
                                                     VK_NULL_HANDLE,
                                                     1,
                                                     &tl_geom,
                                                     nullptr,
                                                     {}};

  VkAccelerationStructureBuildSizesInfoKHR bl_size_info, tl_size_info;
  bl_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  bl_size_info.pNext = nullptr;
  tl_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  tl_size_info.pNext = nullptr;


  uint32_t one = 1;
  std::vector<uint32_t> prim_count(bl_geoms.size());

  std::transform(meshes.begin(), meshes.end(), prim_count.begin(), [](const Mesh &m) { return m.n_index / 3; });

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bl_bgi, prim_count.data(), &bl_size_info);

  vkGetAccelerationStructureBuildSizesKHR(device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tl_bgi, &one, &tl_size_info);

  if(bl_size_info.accelerationStructureSize & 255)
    bl_size_info.accelerationStructureSize = (bl_size_info.accelerationStructureSize & ~255) + 256;

  create_buffer(bl_size_info.accelerationStructureSize + tl_size_info.accelerationStructureSize);

  // bdai.buffer = m_buffer;

  VkAccelerationStructureCreateInfoKHR asci{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, nullptr, 0, m_buffer, 0, bl_size_info.accelerationStructureSize,
                                            VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,          0};

  vk_check(vkCreateAccelerationStructureKHR(device(), &asci, nullptr, &m_blas));

  asci.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  asci.offset = asci.size;
  asci.size = tl_size_info.accelerationStructureSize;


  vk_check(vkCreateAccelerationStructureKHR(device(), &asci, nullptr, &m_tlas));

  scratch_size = std::max(bl_size_info.buildScratchSize, tl_size_info.buildScratchSize);
}

void SimpleTriangleAccelerationStructure::upload_as(const std::vector<Mesh> &meshes, VkCommandPool comp_pool, VkBuffer scratch, VkDeviceAddress scratch_addr)
{
  std::vector<VkAccelerationStructureGeometryKHR> bl_geoms(meshes.size());

  std::transform(meshes.begin(), meshes.end(), bl_geoms.begin(), [&](const Mesh &mesh) -> VkAccelerationStructureGeometryKHR {
    return {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            nullptr,
            VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            {.triangles = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                           nullptr,
                           VK_FORMAT_R32G32B32_SFLOAT,
                           {mesh.vertex_addr},
                           sizeof(vertex_t),
                           mesh.n_vertex - 1,
                           VK_INDEX_TYPE_UINT32,
                           {mesh.index_addr},
                           {.hostAddress = nullptr}}},
            0};
  });

  Core::CommandBuffer cmd_buf(*m_instance, comp_pool);
  cmd_buf.begin_primary();

  VkAccelerationStructureBuildGeometryInfoKHR bgi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                                                  nullptr,
                                                  VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                  0,
                                                  VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                                                  VK_NULL_HANDLE,
                                                  m_blas,
                                                  uint32_t(bl_geoms.size()),
                                                  bl_geoms.data(),
                                                  nullptr,
                                                  {scratch_addr}};

  std::vector<VkAccelerationStructureBuildRangeInfoKHR> asbrs(meshes.size());

  std::transform(meshes.begin(), meshes.end(), asbrs.begin(), [&](const Mesh &mesh) -> VkAccelerationStructureBuildRangeInfoKHR { return {mesh.n_index / 3, 0, 0, 0}; });

  std::vector<VkAccelerationStructureBuildRangeInfoKHR*> p_asbrs(asbrs.size());

  {
    auto asbr_iter = asbrs.begin();
    for(auto &elem: p_asbrs) elem = &*(asbr_iter++);
  }

  cmd_buf.cmd(vkCmdBuildAccelerationStructuresKHR, 1, &bgi, p_asbrs.data());

  VkBufferMemoryBarrier2 bmb{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                             nullptr,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                             0,
                             0,
                             scratch,
                             0,
                             VK_WHOLE_SIZE};

  VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO, nullptr, 0, 0, nullptr, 1, &bmb, 0, nullptr};

  cmd_buf.cmd(vkCmdPipelineBarrier2, &di);

  VkAccelerationStructureDeviceAddressInfoKHR asdai{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, nullptr, m_blas};

  VkDeviceAddress blas_address = vkGetAccelerationStructureDeviceAddressKHR(device(), &asdai);

  Core::Buffer tlas_build_buffer(*m_instance, sizeof(VkAccelerationStructureInstanceKHR),
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  *reinterpret_cast<VkAccelerationStructureInstanceKHR*>(tlas_build_buffer.map()) = {
    VkTransformMatrixKHR{{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}}, 0, 0xFF, 0, 0, blas_address};

  tlas_build_buffer.flush_unmap();

  VkAccelerationStructureGeometryKHR geom{
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    nullptr,
    VK_GEOMETRY_TYPE_INSTANCES_KHR,
    {.instances = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, nullptr, VK_FALSE, {.deviceAddress = tlas_build_buffer.device_address()}}},
    0};


  bgi.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  bgi.dstAccelerationStructure = m_tlas;
  bgi.geometryCount = 1;
  bgi.pGeometries = &geom;

  VkAccelerationStructureBuildRangeInfoKHR asbr{1, 0, 0, 0};
  asbr.primitiveCount = 1;

  auto p_asbr = &asbr;

  cmd_buf.cmd(vkCmdBuildAccelerationStructuresKHR, 1, &bgi, &p_asbr);

  cmd_buf.end();
  m_instance->quick_submit(cmd_buf.cmd_buf(), m_instance->compute_queue());
}
