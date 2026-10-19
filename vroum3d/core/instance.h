#ifndef VROUM3D_CORE_INSTANCE_H_INCLUDED
#define VROUM3D_CORE_INSTANCE_H_INCLUDED

#include "../debug.h"
#include "../utility.h"
#include "../volk_wrapper.h"
#include "allocator.h"
#include "display.hpp"
#include "instance_info.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Core
{

template<bool enable = false>
class InstanceDebugData {
public:
  void destroy(VkInstance) {}

  void create_dbg_mesg(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT &) {}
};

class DefaultInstanceInfo;

template<>
class InstanceDebugData<true> {
public:
  VkHandle<VkDebugUtilsMessengerEXT> m_dbg_messenger;

  void destroy(VkInstance inst)
  {

    m_dbg_messenger.destroy_with([&](auto msg) {
      PFN_vkDestroyDebugUtilsMessengerEXT fn = reinterpret_cast<decltype(fn)>(vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"));
      if(fn) fn(inst, msg, nullptr);
    });
  }

  void create_dbg_mesg(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT &ci);
};

class Instance : public AssignDestroy<Instance>,
                 private InstanceDebugData<debug>,
                 public Allocator {
protected:
  using debug_data_t = InstanceDebugData<debug>;
  /** Base members */

  VkHandle<VkInstance> m_inst;
  VkPhysicalDevice m_pdev;
  VkHandle<VkDevice> m_dev;
  std::uint32_t m_ti, m_gi, m_ci;
  VkQueue m_tq = VK_NULL_HANDLE, m_gq = VK_NULL_HANDLE, m_cq = VK_NULL_HANDLE;
  VkHandle<VkCommandPool> m_transfer_pool;

  struct DelayedDeviceCreation
  {};

public:
  using ExtensionsLayers = DefaultInstanceInfo::ExtensionsLayers;

  static constexpr std::size_t c_frames_in_flight = 2; // AKA number of buffered frames

  template<typename InstanceInfo = DefaultInstanceInfo>
  Instance(InstanceInfo &&ii = {})
  {
    create_instance(ii.inst_exts_lays(), ii.instance_pnext());
    create_dbg_mesg(m_inst, *ii.debug_messenger_info());

    create_device(ii, VK_NULL_HANDLE);
  }

protected:
  /** Create Instance with VkInstance only, to allow surface creation before
   * device creation (for present queue) */
  template<typename InstanceInfo = DefaultInstanceInfo>
  Instance(DelayedDeviceCreation, InstanceInfo &&ii = {})
  {
    create_instance(ii.inst_exts_lays(), ii.instance_pnext());
    create_dbg_mesg(m_inst, *ii.debug_messenger_info());
  }

  template<typename InstanceInfo = DefaultInstanceInfo>
  void create_device(InstanceInfo &&ii = {}, VkSurfaceKHR surf = VK_NULL_HANDLE)
  {
    choose_pdev(ii);

    ii.query_features(m_pdev);

    ii.build_device_pnext();
    const auto &exts = ii.dev_exts();

    set_device_address_enabled(ii.enable_device_address());
    create_device(exts, surf, ii.device_pnext());

    create_transfer_pool();
  }

public:
  void destroy();

  ~Instance() { destroy(); }

  auto graphics_queue() const { return m_gq; }

  auto transfer_queue() const { return m_tq; }

  auto compute_queue() const { return m_cq; }

  auto graphic_queue_index() const { return m_gi; }

  auto transfer_queue_index() const { return m_ti; }

  auto compute_queue_index() const { return m_ci; }

  VkDevice device() const { return m_dev; }

  VkPhysicalDevice pdev() const { return m_pdev; }

  VkCommandPool transfer_pool() const { return m_transfer_pool; }

  void create_transfer_pool();

  /** Quick submit and wait for execution to be done
   * @param cmd_buf Command buffer to submit
   * @param queue Queue to submit to, defaults to graphics queue if equal to
   * VK_NULL_HANDLE
   */
  void quick_submit(VkCommandBuffer cmd_buf, VkQueue queue = VK_NULL_HANDLE);

private:
  void create_instance(ExtensionsLayers &&el, void* instance_pnext = nullptr);

  template<typename InstanceInfo>
  void choose_pdev(InstanceInfo &info);
  void create_device(const std::vector<std::string> &exts, VkSurfaceKHR surf = VK_NULL_HANDLE, void* device_pnext = nullptr);
};

class DisplayInstance : public Instance {

  /** Display associated members */

  VkHandle<VkSurfaceKHR> m_surf;
  VkQueue m_pq;

  VkHandle<VkSwapchainKHR> m_sw;

  struct FrameSync
  {
    VkHandle<VkFence> render_done_fence;
    std::uint32_t image_index;
    VkHandle<VkSemaphore> image_avail_sem;

    VkHandle<VkImage> depth_image;
    VkHandle<VkImageView> depth_view;
    owned_memory_t depth_mem;
  };

  std::array<FrameSync, c_frames_in_flight> m_frame_sync; // Frame sync primitives, one per buffered frame

  struct SwapchainFrame
  {
    VkImage img;
    VkHandle<VkImageView> view;
    VkHandle<VkSemaphore> render_done_sem;
  };

  std::vector<SwapchainFrame> m_sw_frames;

  VkFormat m_depth_format;
  VkSurfaceFormatKHR m_sw_format;
  VkPresentModeKHR m_sw_pres_mode;
  std::uint32_t m_w, m_h;
  std::uint32_t m_next_frame = 0; // Index of next frame in m_frame_sync vector
  VkImageUsageFlags m_depth_image_usage;
  bool m_resized = false;

public:
  /** Check if window was resized. Resets resize flag, such that next calls will
   * return false until new resize. */
  bool resized()
  {
    bool tmp = m_resized;
    m_resized = false;
    return tmp;
  }

  auto present_queue() const { return m_pq; }

  auto w() const { return m_w; }

  auto h() const { return m_h; }

  float aspect_ratio() const { return float(m_w) / float(m_h); }

  std::uint32_t n_swapchain_images() const { return m_sw_frames.size(); }

  VkImageView sw_view(std::uint32_t idx) const { return m_sw_frames[idx].view; }

  VkImage sw_image(std::uint32_t idx) const { return m_sw_frames[idx].img; }

  VkImageView depth_view() const { return m_frame_sync[m_next_frame].depth_view; }

  VkImage depth_image() const { return m_frame_sync[m_next_frame].depth_image; }

  const VkFormat &color_format() const { return m_sw_format.format; }

  const VkFormat &depth_format() const { return m_depth_format; }

  ~DisplayInstance() { destroy(); }

  template<typename InstanceInfo>
  DisplayInstance(Display &disp, InstanceInfo &&ii) : Instance(DelayedDeviceCreation(), ii)
  {
    create_surf(disp.m_wind);

    ii.set_surface(m_surf);

    create_device(ii, m_surf);
    get_present_queue();

    create_sw();
    create_sw_views();
    find_depth_format();

    m_depth_image_usage = ii.depth_image_additional_usage() | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    create_frame_sync();
  }

  DisplayInstance(Display &disp) : DisplayInstance(disp, DisplayInstanceInfo(DefaultInstanceInfo())) {}

  void destroy();

  void create_sw();

  bool acquire_next_image()
  {
    auto res = vkAcquireNextImageKHR(m_dev, m_sw, 0, m_frame_sync[m_next_frame].image_avail_sem, VK_NULL_HANDLE, &m_frame_sync[m_next_frame].image_index);

    switch(res)
    {
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR: return true;
    case VK_TIMEOUT:
    case VK_NOT_READY: return false;
    default: vk_check(res); return true;
    }
  }

  /* Index of next frame among swapchain frames. Only valid after a successful
   * call to acquire_next_image and before render submission */
  std::uint32_t next_swapchain_frame() { return m_frame_sync[m_next_frame].image_index; }

  std::uint32_t next_frame() { return m_next_frame; }

  void submit_render_present(VkCommandBuffer cmd_buf);

  bool render_done() const
  {
    auto res = vkWaitForFences(m_dev, 1, &m_frame_sync[m_next_frame].render_done_fence, VK_TRUE, 0);

    if(res > 0) return false;

    vk_check(res);

    return true;
  }

  void begin_rendering(VkCommandBuffer buffer, bool secondary_contents = false, VkClearColorValue clear_color_value = {{0.0f, 0.0f, 0.0f, 0.0f}});
  void end_rendering(VkCommandBuffer cmd_buf);

  void set_dynamic_viewport_scissor(VkCommandBuffer cmd_buf)
  {
    VkRect2D sc{{0, 0}, {w(), h()}};
    VkViewport vp{0.0, 0.0, float(w()), float(h()), 0.0f, 1.0f};

    vkCmdSetViewport(cmd_buf, 0, 1, &vp);
    vkCmdSetScissor(cmd_buf, 0, 1, &sc);
  }

  /* Wait for all renders to be done.
   * Do not use in render loop, or it may lead to poor performance. */

  void wait_renders_done()
  {
    std::array<VkFence, c_frames_in_flight> fences;
    for(std::uint32_t i(0); i != c_frames_in_flight; ++i) fences[i] = m_frame_sync[i].render_done_fence;

    vk_check(vkWaitForFences(device(), c_frames_in_flight, fences.data(), VK_TRUE, UINT64_MAX));
  }

private:
  void get_present_queue();

  void find_sw_info();

  void create_surf(SDL_Window* wind);

  void create_sw_views();
  void create_depth_images();

  void find_depth_format();
  void create_transfer_pool();

  /** Takes needed extensions and layers as arg and adds extensions required by
   * layers and by the SDL_Window of display */
  void fill_exts_lays(ExtensionsLayers &el);

  void create_fence();

  void resize();

  void destroy_swapchain();
  void destroy_depth_images();

  void create_frame_sync();
};

template<typename InstanceInfo>
void Instance::choose_pdev(InstanceInfo &info)
{
  std::vector<VkPhysicalDevice> pdevs = wrap_enumerate(vkEnumeratePhysicalDevices, m_inst);

  check(pdevs.size());

  VkPhysicalDevice best_pdev(VK_NULL_HANDLE);
  int64_t best_score(-1);

  for(auto pdev: pdevs)
  {
    int64_t score = info.rate_physical_device(pdev);
    if(score > best_score)
    {
      best_pdev = pdev;
      best_score = score;
    }
  }

  if(best_score < 0) throw std::runtime_error("No compatible physical device found.");

  m_pdev = best_pdev;
}

} // namespace Vroum3d::Core

#endif
