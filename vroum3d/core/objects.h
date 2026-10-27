#ifndef VROUM3D_CORE_OBJECTS_H_INCLUDED
#define VROUM3D_CORE_OBJECTS_H_INCLUDED

#include "../utility.h"
#include "../volk_wrapper.h"
#include "allocator.h"
#include "instance.h"
#include "vkutil.h"

#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

class MemoryBackedObject : public AssignDestroy<MemoryBackedObject> {
public:
  MemoryBackedObject(Allocator &allocator) : m_allocator(&allocator) {}

  MemoryBackedObject(MemoryBackedObject &&) = default;
  MemoryBackedObject &operator=(MemoryBackedObject &&) = default;

  ~MemoryBackedObject() { destroy(); }

  void destroy()
  {
    if(m_allocator) m_allocator->free(m_mem);
  }

  VkDevice device() const { return m_allocator->device(); }

  char* map() { return m_allocator->map(m_mem); }

  void flush_unmap()
  {
    m_allocator->flush(m_mem);
    m_allocator->unmap(m_mem);
  }

  void flush() { m_allocator->flush(m_mem); }

  void unmap() { m_allocator->unmap(m_mem); }

  void invalidate() { m_allocator->invalidate(m_mem); }

protected:
  Allocator* m_allocator;
  Allocator::owned_memory_t m_mem;
};

/** Class for a simple buffer, owns the buffer */
class Buffer : public MemoryBackedObject,
               public AssignDestroy<Buffer> {

  static constexpr VkDeviceSize c_max_atom_size = 256;

public:
  Buffer(Buffer &&) = default;
  Buffer &operator=(Buffer &&) = default;

  /** Buffer constructor : creates buffer and allocs mem
   * \param alloc Allocator to create buffer with
   * \param bs Buffer size
   * \param use Buffer usage
   * \param mp Memory properties of the memory to bind to the buffer
   * \param alignment Requested memory alignment. Should be a power of 2. Defaults to 0, i.e. minimal according to buffer's requirements.
   */

  Buffer(Allocator &alloc, VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags mp, VkDeviceSize alignment = 0) : MemoryBackedObject(alloc)
  { create_buffer(bs, use, mp, alignment); }

  using MemoryBackedObject::MemoryBackedObject;

  void destroy()
  {
    m_buffer.destroy_with([&](auto buf) { vkDestroyBuffer(device(), buf, nullptr); });
    MemoryBackedObject::destroy();
  }

  VkDeviceAddress device_address() const
  {
    VkBufferDeviceAddressInfoKHR dai{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_buffer};

    return vkGetBufferDeviceAddress(device(), &dai);
  }

  ~Buffer() { destroy(); }

  const VkBuffer &buffer() const { return m_buffer; }

  operator VkBuffer() const { return buffer(); }

  /**
   * Create Buffer
   *
   * Create Buffer. Do not create if buffer already exists without destroy prior.
   *
   * \param bs Buffer size
   * \param use Buffer usage
   * \param mp Memory properties of the memory to bind to the buffer
   * \param alignment Requested memory alignment. Should be a power of 2. Defaults to 0, i.e. minimal according to buffer's requirements.
   */
  void create_buffer(VkDeviceSize bs, VkBufferUsageFlags use, VkMemoryPropertyFlags mp, VkDeviceSize alignment = 0)
  {
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, bs, use, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr};

    vk_check(vkCreateBuffer(device(), &bi, nullptr, &m_buffer));

    m_mem = m_allocator->allocate_bind(m_buffer, mp, alignment);
  }

private:
  VkHandle<VkBuffer> m_buffer;
};

class Image : public MemoryBackedObject,
              public AssignDestroy<Image> {
public:
  Image(Image &&) = default;
  Image &operator=(Image &&) = default;

  using MemoryBackedObject::MemoryBackedObject;

  Image(Allocator &alloc, VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, uint32_t miplevels = 1,
        uint32_t arraylayers = 1, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
        VkImageLayout init_layout = VK_IMAGE_LAYOUT_UNDEFINED) : MemoryBackedObject(alloc)
  { create_image(type, format, extent, usage, mem_props, miplevels, arraylayers, tiling, sample_count, init_layout); }

  void create_image(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, uint32_t miplevels = 1,
                    uint32_t arraylayers = 1, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
                    VkImageLayout init_layout = VK_IMAGE_LAYOUT_UNDEFINED)
  {
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0,       type,       format, extent, miplevels, arraylayers, sample_count, tiling, usage,
                          VK_SHARING_MODE_EXCLUSIVE,           0,       nullptr, init_layout};

    vk_check(vkCreateImage(m_allocator->device(), &ici, nullptr, &m_image));

    m_mem = m_allocator->allocate_bind(m_image, mem_props);
  }

  VkImage image() const { return m_image; }

  operator VkImage() const { return image(); }

  void destroy()
  {
    m_image.destroy_with([&](VkImage im) { vkDestroyImage(device(), im, nullptr); });
    MemoryBackedObject::destroy();
  }

  ~Image() { destroy(); }

private:
  Allocator* m_allocator;
  VkHandle<VkImage> m_image;
  Allocator::owned_memory_t m_mem;
};

class ImageAndView : public Image,
                     public AssignDestroy<ImageAndView> {
public:
  ImageAndView(ImageAndView &&) = default;
  ImageAndView &operator=(ImageAndView &&) = default;

  ImageAndView(Allocator alloc, VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, uint32_t miplevels = 1,
               uint32_t arraylayers = 1, const VkComponentMapping &mapping = vkutil::components_id,
               const VkImageSubresourceRange &subresrange = vkutil::color_subres_plain, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
               VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT, VkImageLayout init_layout = VK_IMAGE_LAYOUT_UNDEFINED) :
    Image(alloc, type, format, extent, usage, mem_props, miplevels, arraylayers, tiling, sample_count, init_layout)
  { create_view(vkutil::view_type_from_image_type(type), format, mapping, subresrange); }

  ImageAndView(Allocator &alloc) : Image(alloc) {}

  operator VkImageView() const { return m_view; }

  VkImageView view() const { return m_view; }

  void create_view(VkImageViewType type, VkFormat format, const VkComponentMapping &mapping = vkutil::components_id,
                   const VkImageSubresourceRange &subresrange = vkutil::color_subres_plain)
  {
    VkImageViewCreateInfo iwci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, image(), type, format, mapping, subresrange};

    vk_check(vkCreateImageView(device(), &iwci, nullptr, &m_view));
  }

  void destroy()
  {
    m_view.destroy_with([&](auto v) { vkDestroyImageView(device(), v, nullptr); });
    Image::destroy();
  }

  void create_image_and_view(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, uint32_t miplevels = 1,
                             uint32_t arraylayers = 1, const VkComponentMapping &mapping = vkutil::components_id,
                             const VkImageSubresourceRange &subresrange = vkutil::color_subres_plain, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                             VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT, VkImageLayout init_layout = VK_IMAGE_LAYOUT_UNDEFINED)
  {
    create_image(type, format, extent, usage, mem_props, miplevels, arraylayers, tiling, sample_count, init_layout);
    create_view(vkutil::view_type_from_image_type(type), format, mapping, subresrange);
  }

  ~ImageAndView() { destroy(); }

private:
  VkHandle<VkImageView> m_view;
};

class CommandPool : public AssignDestroy<CommandPool> {
public:
  CommandPool(const Instance &inst, uint32_t queue_index, VkCommandPoolCreateFlags create_flags = 0) : m_device(inst.device())
  {
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, create_flags, queue_index};

    vk_check(vkCreateCommandPool(m_device, &pi, nullptr, &m_command_pool));
  }

  VkCommandPool command_pool() const { return m_command_pool; }

  operator VkCommandPool() const { return command_pool(); }

  void destroy()
  {
    m_command_pool.destroy_with([&](auto cmdp) { vkDestroyCommandPool(m_device, cmdp, nullptr); });
  }

  ~CommandPool() { destroy(); }

private:
  VkDevice m_device;
  VkHandle<VkCommandPool> m_command_pool;
};

template<typename HandleType = VkCommandBuffer>
class CommandBufferRef {
public:
  void begin_primary();

  void begin_secondary_rendering(DisplayInstance &inst);

  void end() { vkEndCommandBuffer(m_cmd_buf); }

  template<typename CmdFun, typename... Args>
  void cmd(CmdFun cf, Args &&... ags)
  { cf(m_cmd_buf, std::forward<Args>(ags)...); }

  const VkCommandBuffer &cmd_buf() const { return m_cmd_buf; }

  operator VkCommandBuffer() const { return cmd_buf(); }

  void begin_rendering(DisplayInstance &inst) { inst.begin_rendering(m_cmd_buf); }

  void end_rendering(DisplayInstance &inst) { inst.end_rendering(m_cmd_buf); }

  void bind_graphics_pipeline(DisplayInstance &inst, VkPipeline pipe);

  void reset() { vkResetCommandBuffer(m_cmd_buf, 0); }

  CommandBufferRef() : m_cmd_buf(VK_NULL_HANDLE) {}

  template<typename HandleFrom = const VkCommandBuffer &>
  CommandBufferRef(HandleFrom &&from) : m_cmd_buf(std::forward<HandleFrom>(from))
  {}

protected:
  HandleType m_cmd_buf;
};

class CommandBuffer : public AssignDestroy<CommandBuffer>,
                      public CommandBufferRef<VkHandle<VkCommandBuffer>> {
public:
  CommandBuffer() {}

  ~CommandBuffer() { destroy(); }

  CommandBuffer(CommandBuffer &&) = default;

  CommandBuffer(const Instance &inst, VkCommandPool cmd_pool, bool primary = true) : m_dev(inst.device()), m_cmd_pool(cmd_pool) { allocate_command_buffer(primary); }

  CommandBuffer(const Instance &inst, bool primary = true) : CommandBuffer(inst, inst.transfer_pool(), primary) {}

  void destroy()
  {
    m_cmd_buf.destroy_with([&](auto buf) { vkFreeCommandBuffers(m_dev, m_cmd_pool, 1, &buf); });
  }

private:
  void allocate_command_buffer(bool primary = true)
  {
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, m_cmd_pool,
                                   primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, 1};

    vk_check(vkAllocateCommandBuffers(m_dev, &ai, &m_cmd_buf));
  }

  VkDevice m_dev;
  VkCommandPool m_cmd_pool;
};

class CommandBufferVector : public AssignDestroy<CommandBufferVector>,
                            private std::vector<VkCommandBuffer> {
  using vector_t = std::vector<VkCommandBuffer>;

public:
  using vector_t::begin, vector_t::end, vector_t::cbegin, vector_t::cend, vector_t::size, vector_t::data;

  CommandBufferVector() = default;

  CommandBufferVector(const Instance &inst, VkCommandPool cmd_pool, uint32_t n_buffer, bool primary = true) : m_dev(inst.device()), m_cmd_pool(cmd_pool)
  { allocate_command_buffers(n_buffer, primary); }

  void destroy()
  {
    if(!empty())
    {
      vkFreeCommandBuffers(m_dev, m_cmd_pool, uint32_t(size()), data());

      clear();
    }
  }

  CommandBufferRef<> at(uint32_t idx) const { return vector_t::at(idx); }

  auto operator[](uint32_t idx) const { return at(idx); }

private:
  void allocate_command_buffers(uint32_t n_buffer, bool primary = true)
  {
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, m_cmd_pool,
                                   primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, n_buffer};

    resize(n_buffer);

    vk_check(vkAllocateCommandBuffers(m_dev, &ai, data()));
  }

  VkDevice m_dev;
  VkCommandPool m_cmd_pool;
};

class Fence : public AssignDestroy<Fence> {
public:
  Fence() {}

  Fence(const Instance &inst) : m_device(inst.device())
  {
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};

    vk_check(vkCreateFence(m_device, &fi, nullptr, &m_fence));
  }

  const VkFence &fence() const { return m_fence; }

  auto wait() { vk_check(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, ~(0))); }

  ~Fence() { destroy(); }

  void destroy()
  {
    m_fence.destroy_with([&](auto fnc) { vkDestroyFence(m_device, fnc, nullptr); });
  }

private:
  VkDevice m_device;
  VkHandle<VkFence> m_fence;
};

/** Descriptor pool */
class DescriptorPool : public AssignDestroy<DescriptorPool> {
public:
  DescriptorPool(Instance &inst, uint32_t max_sets, const std::vector<VkDescriptorPoolSize> &pool_sizes) : m_device(inst.device())
  {
    VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, 0, max_sets, uint32_t(pool_sizes.size()), pool_sizes.data()};

    vk_check(vkCreateDescriptorPool(m_device, &info, nullptr, &m_descriptor_pool));
  }

  void destroy()
  {
    m_descriptor_pool.destroy_with([&](auto dp) { vkDestroyDescriptorPool(m_device, dp, nullptr); });
  }

  ~DescriptorPool() { destroy(); }

  template<typename ContainerOut, typename ContainerIn = std::initializer_list<VkDescriptorSetLayout>>
  void allocate_descriptor_sets(const ContainerIn &layouts, ContainerOut &sets_out, const void* pnext = nullptr)
  {
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, pnext, m_descriptor_pool, uint32_t(layouts.size()), std::data(layouts)};

    vk_check(vkAllocateDescriptorSets(m_device, &ai, sets_out.data()));
  }

  template<typename ContainerOut, typename VarSizes = std::initializer_list<uint32_t>, std::enable_if_t<!std::is_pointer_v<VarSizes>, bool> = true,
           typename ContainerIn = std::initializer_list<VkDescriptorSetLayout>>
  void allocate_descriptor_sets(const ContainerIn &layouts, ContainerOut &sets_out, const VarSizes &var_sizes)
  {
    VkDescriptorSetVariableDescriptorCountAllocateInfo vsi{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, nullptr, uint32_t(var_sizes.size()),
                                                           std::data(var_sizes)};

    allocate_descriptor_sets(layouts, sets_out, &vsi);
  }

  VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout lay)
  {
    std::array<VkDescriptorSetLayout, 1> lay_cont{{lay}};
    std::array<VkDescriptorSet, 1> out;

    allocate_descriptor_sets(lay_cont, out);

    return out[0];
  }

  VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout lay, uint32_t varsize)
  {
    std::array<uint32_t, 1> varsize_arr{{varsize}};
    std::array<VkDescriptorSetLayout, 1> lay_cont{{lay}};
    std::array<VkDescriptorSet, 1> out;

    allocate_descriptor_sets(lay_cont, out, varsize_arr);

    return out[0];
  }

private:
  VkDevice m_device;
  VkHandle<VkDescriptorPool> m_descriptor_pool;
};

template<typename HandleType>
void CommandBufferRef<HandleType>::begin_primary()
{
  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

  vk_check(vkBeginCommandBuffer(m_cmd_buf, &bi));
}

template<typename HandleType>
void CommandBufferRef<HandleType>::begin_secondary_rendering(DisplayInstance &inst)
{

  VkCommandBufferInheritanceRenderingInfo inhri{
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO, nullptr, 0, 0, 1, &inst.color_format(), inst.depth_format(), VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT};

  VkCommandBufferInheritanceInfo inhi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, &inhri, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, VK_FALSE, 0, 0};

  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
                              VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, &inhi};

  vk_check(vkBeginCommandBuffer(m_cmd_buf, &bi));
}

template<typename HandleType>
void CommandBufferRef<HandleType>::bind_graphics_pipeline(DisplayInstance &inst, VkPipeline pipe)
{
  vkCmdBindPipeline(m_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

  inst.set_dynamic_viewport_scissor(m_cmd_buf);
}

} // namespace Vroum3d::Core
#endif
