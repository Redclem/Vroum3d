#ifndef VROUM3D_CORE_UPDATE_DESCRIPTOR_SET
#define VROUM3D_CORE_UPDATE_DESCRIPTOR_SET

#include "../volk_wrapper.h"
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core::UpdateDescriptorSet
{

struct SetLocation 
{
  SetLocation(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement) : 
  m_dstSet(dstSet), m_dstBinding(dstBinding), m_dstArrayElement(dstArrayElement) {}

  VkDescriptorSet m_dstSet;
  uint32_t m_dstBinding, m_dstArrayElement;

  VkWriteDescriptorSet get_write_skeleton() const
  {
    VkWriteDescriptorSet res;
    res.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    res.pNext = nullptr;
    res.dstSet = m_dstSet;
    res.dstBinding = m_dstBinding;
    res.dstArrayElement = m_dstArrayElement;

    return res;
  };
};

struct DescriptorWriteBase {};

template<typename BuffersStorage = std::initializer_list<VkDescriptorBufferInfo>>
struct WriteBuffer : public DescriptorWriteBase
{
  WriteBuffer(SetLocation location, VkDescriptorType buffer_type, BuffersStorage&& from) : m_location(location), m_buffer_type(buffer_type), buffers(std::forward<BuffersStorage>(from)) {}

  SetLocation m_location;
  VkDescriptorType m_buffer_type;
  std::remove_cvref_t<BuffersStorage> buffers;

  VkWriteDescriptorSet export_write() const
  {
    VkWriteDescriptorSet write = m_location.get_write_skeleton();

    write.descriptorType = m_buffer_type;
    write.descriptorCount = buffers.size();
    write.pBufferInfo = buffers.data();
  
    return write;
  }
};


template<typename Write0, typename ... Writes>
void assemble_writes(const Write0& write0, const Writes& ... writes, VkWriteDescriptorSet* dst)
{
  *dst = write0.export_write();
  
  if constexpr(sizeof...(Writes))
    assemble_writes(writes..., dst + 1);
}

template<typename ... Writes>
void update_descriptor_sets(VkDevice dev, const Writes& ... writes)
{
  std::array<VkWriteDescriptorSet, sizeof...(Writes)> des_writes;

  assemble_writes(writes..., des_writes.data());

  vkUpdateDescriptorSets(dev, des_writes.size(), des_writes.data(), 0, nullptr);
}

}

#endif
