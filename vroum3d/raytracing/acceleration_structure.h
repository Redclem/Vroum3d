#ifndef VROUM3D_RAYTRACING_ACCELERATION_STRUCTURE_H_INCLUDED
#define VROUM3D_RAYTRACING_ACCELERATION_STRUCTURE_H_INCLUDED

#include "../core/instance.h"
#include "../math/vec.hpp"
#include <cstdint>
#include "../volk_wrapper.h"


namespace Vroum3d::Raytracing
{

class SimpleTriangleAccelerationStructure : public AssignDestroy<SimpleTriangleAccelerationStructure>
{
public:
  

  SimpleTriangleAccelerationStructure(Core::Instance& instance, VkBuffer data_source, uint32_t n_vertex, uint32_t n_index) : SimpleTriangleAccelerationStructure(instance) {
    create_as(data_source, n_vertex, n_index);
  }


  SimpleTriangleAccelerationStructure(Core::Instance& instance) : m_instance(&instance), m_device(m_instance->device()) {}

  auto device() const {return m_device;}

  using index_t = std::uint32_t;
  using vertex_t = Math::vec3;

  void create_as(VkBuffer data_source, uint32_t n_vertex, uint32_t n_index);

  void destroy();

  ~SimpleTriangleAccelerationStructure() {destroy();}

  void create_buffer(VkDeviceSize size);


private:

  Core::Instance* m_instance;
  VkDevice m_device;

  VkHandle<VkBuffer> m_buffer;
  Core::Allocator::OwnedMemory m_buffer_mem;

  VkHandle<VkAccelerationStructureKHR> m_blas, m_tlas;
};

}

#endif
