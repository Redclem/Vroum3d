#ifndef VROUM3D_CORE_UPDATE_DESCRIPTOR_SET
#define VROUM3D_CORE_UPDATE_DESCRIPTOR_SET

#include "../volk_wrapper.h"

#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core::UpdateDescriptorSet
{

struct SetLocation
{
  SetLocation(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement) : m_dstSet(dstSet), m_dstBinding(dstBinding), m_dstArrayElement(dstArrayElement) {}

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

struct DescriptorWriteBase
{};

template<typename BuffersStorage = std::initializer_list<VkDescriptorBufferInfo>>
struct WriteBuffers : public DescriptorWriteBase
{
  WriteBuffers(SetLocation location, VkDescriptorType buffer_type, BuffersStorage &&from) :
    m_location(location), m_type(buffer_type), m_buffers(std::forward<BuffersStorage>(from))
  {}

  SetLocation m_location;
  VkDescriptorType m_type;
  std::remove_cvref_t<BuffersStorage> m_buffers;

  VkWriteDescriptorSet export_write() const
  {
    VkWriteDescriptorSet write = m_location.get_write_skeleton();

    write.descriptorType = m_type;
    write.descriptorCount = m_buffers.size();
    write.pBufferInfo = std::data(m_buffers);

    return write;
  }
};

struct DescriptorImageInfo : VkDescriptorImageInfo
{
  DescriptorImageInfo(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    imageView = view;
    imageLayout = layout;
  }

  DescriptorImageInfo(VkSampler samp) { sampler = samp; }

  DescriptorImageInfo(VkSampler samp, VkImageView view, VkImageLayout lay = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    sampler = samp;
    imageView = view;
    imageLayout = lay;
  }
};

static_assert(sizeof(DescriptorImageInfo) == sizeof(VkDescriptorImageInfo), "Error with DescriptorImageInfo structure layout");

template<typename ImagesStorage = std::initializer_list<DescriptorImageInfo>>
struct WriteImages
{
  SetLocation m_location;
  VkDescriptorType m_type;
  std::remove_cvref_t<ImagesStorage> m_images;

  VkWriteDescriptorSet export_write() const
  {
    auto write = m_location.get_write_skeleton();
    write.descriptorType = m_type;
    write.descriptorCount = m_images.size();
    write.pImageInfo = std::data(m_images);

    return write;
  }
};

template<typename AcsStorage = std::initializer_list<VkAccelerationStructureKHR>>
struct WriteAccelerationStructure
{
  WriteAccelerationStructure(SetLocation location, AcsStorage &&acc_structs) :
    m_location(location), m_acc_structs(std::forward<AcsStorage>(acc_structs)),
    m_write_acc({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, nullptr, uint32_t(m_acc_structs.size()), std::data(m_acc_structs)})
  {}

  SetLocation m_location;
  std::remove_cvref_t<AcsStorage> m_acc_structs;
  VkWriteDescriptorSetAccelerationStructureKHR m_write_acc;

  VkWriteDescriptorSet export_write() const
  {
    auto write = m_location.get_write_skeleton();
    write.descriptorCount = m_acc_structs.size();
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pNext = &m_write_acc;

    return write;
  }
};

template<typename Write>
void assemble_writes(VkWriteDescriptorSet* wds, Write &&write)
{ *wds = write.export_write(); }

template<typename Write0, typename... Writes>
void assemble_writes(VkWriteDescriptorSet* wds, Write0 &&write0, Writes &&... writes)
{
  *wds = write0.export_write();

  assemble_writes(wds + 1, std::forward<Writes>(writes)...);
}

template<typename... Writes>
void update_descriptor_sets(VkDevice dev, Writes &&... writes)
{
  std::array<VkWriteDescriptorSet, sizeof...(Writes)> des_writes;

  assemble_writes(des_writes.data(), std::forward<Writes>(writes)...);

  vkUpdateDescriptorSets(dev, des_writes.size(), std::data(des_writes), 0, nullptr);
}

} // namespace Vroum3d::Core::UpdateDescriptorSet

#endif
