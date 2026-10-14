#ifndef VROUM3D_RAYTRACING_ACCELERATION_STRUCTURE_H_INCLUDED
#define VROUM3D_RAYTRACING_ACCELERATION_STRUCTURE_H_INCLUDED

#include "../core/instance.h"
#include "../math/vec.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Raytracing
{

class AccelerationStructure : public AssignDestroy<AccelerationStructure>
{
public:
  

  AccelerationStructure(Core::Instance& instance, VkBuffer data_source, uint32_t n_vertex, uint32_t n_index) : AccelerationStructure(instance) {
    create_as(data_source, n_vertex, n_index);
  }


  AccelerationStructure(Core::Instance& instance) : m_instance(&instance), m_device(m_instance->device()) {}

  auto device() const {return m_device;}

  using index_t = std::uint32_t;
  using vertex_t = Math::vec3;

  void create_as(VkBuffer data_source, uint32_t n_vertex, uint32_t n_index);

  void destroy();

  ~AccelerationStructure() {destroy();}

  void create_buffer(VkDeviceSize size);


private:

  Core::Instance* m_instance;
  VkDevice m_device;

  VkHandle<VkBuffer> m_buffer;
  Core::Allocator::OwnedMemory m_buffer_mem;

  VkHandle<VkAccelerationStructureKHR> m_blas;
};

}

#endif
