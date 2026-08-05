#ifndef VROUM3D_CORE_INSTANCE_H_INCLUDED
#define VROUM3D_CORE_INSTANCE_H_INCLUDED

#include "../utility.h"
#include "../debug.h"
#include "display.hpp"
#include "allocator.h"

#include <SDL3/SDL.h>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace Vroum3d::Core
{

#ifdef VROUM3D_ENABLE_SYNCVAL
constexpr bool c_enable_syncval = true;
#else
constexpr bool c_enable_syncval = false;
#endif



template<bool enable = false>
class InstanceDebugData {
public:
	void destroy(VkInstance) {}
  void create_dbg_mesg(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT&) {}
};

struct DefaultInstanceInfo;

template<>
class InstanceDebugData<true>
{
public:
	VkHandle<VkDebugUtilsMessengerEXT> m_dbg_messenger;
	void destroy(VkInstance inst)
	{

		m_dbg_messenger.destroy_with([&](auto msg){
			PFN_vkDestroyDebugUtilsMessengerEXT fn = reinterpret_cast<decltype(fn)>(vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"));
			if(fn) fn(inst, msg, nullptr);
		});
	}

  void create_dbg_mesg(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT& ci);
};

class Instance : public AssignDestroy<Instance>, private InstanceDebugData<debug>, public Allocator
{
protected:
	using debug_data_t = InstanceDebugData<debug>;
  /** Base members */

	VkHandle<VkInstance> m_inst;
	VkPhysicalDevice m_pdev;
	VkHandle<VkDevice> m_dev;
	std::uint32_t m_ti, m_gi;
	VkQueue m_tq = VK_NULL_HANDLE, m_gq = VK_NULL_HANDLE;
	VkHandle<VkCommandPool> m_transfer_pool;

	struct DelayedDeviceCreation {};


public:

	struct ExtensionsLayers
	{
		std::vector<std::string> exts, lays;
	};

	template<typename InstanceInfo = DefaultInstanceInfo>
  Instance(InstanceInfo&& ii = {})
  {
		create_instance(ii.inst_exts_lays(), ii.instance_pnext());
    create_dbg_mesg(m_inst, *ii.debug_messenger_info());

		choose_pdev();
		create_device(ii.dev_exts(), VK_NULL_HANDLE, ii.device_pnext());

		create_transfer_pool();

	}
protected:
	/** Create Instance with VkInstance only, to allow surface creation before device creation (for present queue) */
	template<typename InstanceInfo = DefaultInstanceInfo>
  Instance(DelayedDeviceCreation, InstanceInfo&& ii = {})
	{
		create_instance(ii.inst_exts_lays(), ii.instance_pnext());
    create_dbg_mesg(m_inst, *ii.debug_messenger_info());
	}

	template<typename InstanceInfo = DefaultInstanceInfo>
	void create_device(InstanceInfo&& ii = {}, VkSurfaceKHR surf = VK_NULL_HANDLE)
	{
		choose_pdev();
		create_device(ii.dev_exts(), surf, ii.device_pnext());

		create_transfer_pool();
	}

public:

  void destroy();

  ~Instance() {destroy();}

  auto graphics_queue() const {return m_gq;}
  auto transfer_queue() const {return m_tq;}

	auto graphic_queue_index() const {return m_gi;}
	auto transfer_queue_index() const {return m_ti;}

	VkDevice device() const {return m_dev;}
	VkPhysicalDevice pdev() const {return m_pdev;}
	VkCommandPool transfer_pool() const {return m_transfer_pool;}
	void create_transfer_pool();

	void quick_submit(VkCommandBuffer cmd_buf);
private:


	void create_instance(ExtensionsLayers&& el, void* instance_pnext = nullptr);

	void choose_pdev();
	void create_device(const std::vector<std::string>& exts, VkSurfaceKHR surf = VK_NULL_HANDLE, void* device_pnext = nullptr);
};

template<bool debug_enable>
struct InstanceInfoDebug
{
  void* instance_pnext() const {return nullptr;}

  VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_info() const {return nullptr;}
};

template<>
struct InstanceInfoDebug<true>
{
  constexpr static VkBool32 c_sync_val_enabled = VK_TRUE;
  constexpr static std::array c_layer_settings = [](){
    if constexpr (c_enable_syncval) return std::array<VkLayerSettingEXT, 1>{{{
    "VK_LAYER_KHRONOS_validation",
    "validate_sync",
    VK_LAYER_SETTING_TYPE_BOOL32_EXT,
    1,
    &c_sync_val_enabled
  }}};
    else return std::array<VkLayerSettingEXT, 0>();
  }();

  VkLayerSettingsCreateInfoEXT lays_s = {
    VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
    nullptr,
    c_layer_settings.size(),
    c_layer_settings.data()
  };

  static VkBool32 callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT sever,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* cbd,
    void*) 
  {
    Vroum3d::log("Vulkan Message:");
    Vroum3d::log("Severity:", string_VkDebugUtilsMessageSeverityFlagsEXT(sever));
    Vroum3d::log("Type:", string_VkDebugUtilsMessageTypeFlagsEXT(type));
    Vroum3d::log(cbd->pMessage, "\n\n");
    return VK_FALSE;
  };

  VkDebugUtilsMessengerCreateInfoEXT dbi = {
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
    callback,
    nullptr
  };

  InstanceInfoDebug() {}

  void* instance_pnext() {
    if constexpr(c_layer_settings.size() > 0) dbi.pNext = &lays_s;
    return reinterpret_cast<void*>(&dbi);
  }
  const VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_info() {return &dbi;}
};

struct DefaultInstanceInfo : InstanceInfoDebug<debug>
{
	template<bool dbg = false>
	struct DefaultInstanceExtensions {
		static constexpr std::array value = [](){
      if constexpr (dbg && c_enable_syncval) return std::array<const char*, 1>{"VK_EXT_layer_settings"};
      else return std::array<const char*, 0>{};
    }();
	};

	template<bool dbg = false>
	struct DefaultInstanceLayers {
		static constexpr std::array value = [](){
			if constexpr (dbg) return std::array<const char*, 1>{"VK_LAYER_KHRONOS_validation"};
			else return std::array<const char*, 0>{};

		}();
	};

	Instance::ExtensionsLayers inst_exts_lays() 
	{	
		constexpr auto extarray = DefaultInstanceExtensions<debug>::value;
		constexpr auto layarray = DefaultInstanceLayers<debug>::value;

		return {{extarray.begin(), extarray.end()} ,
			{layarray.begin(), layarray.end()}};
	}

	std::vector<std::string> dev_exts() {return {};}

  void* device_pnext() {return nullptr;}

  using InstanceInfoDebug<debug>::instance_pnext;
};

class DisplayInstance : public Instance {

  /** Display associated members */
	
  VkHandle<VkSurfaceKHR> m_surf;
	VkQueue m_pq;
	VkHandle<VkSwapchainKHR> m_sw;

  std::vector<VkImage> m_sw_images;
	std::vector<VkHandle<VkImageView>> m_sw_views;
	VkHandle<VkImage> m_depth_image;

	VkHandle<VkImageView> m_depth_view;
  owned_memory_t m_depth_mem;

	VkHandle<VkSemaphore> m_image_avail_sem;
	std::vector<VkHandle<VkSemaphore>> m_render_done_sems;

	VkHandle<VkFence> m_render_done_fence;

	VkFormat m_depth_format;
	VkSurfaceFormatKHR m_sw_format;
	VkPresentModeKHR m_sw_pres_mode;
	std::uint32_t m_w, m_h;
  VkImageUsageFlags m_depth_image_usage;
  bool m_resized = false;

public:

  /** Check if window was resized. Resets resize flag, such that next calls will return false until new resize. */
  bool resized() {
    bool tmp = m_resized;
    m_resized = false;
    return tmp;
  }

  const VkFence& render_done_fence() const {return m_render_done_fence;}

  auto present_queue() const {return m_pq;}

	auto w() const {return m_w;}
	auto h() const {return m_h;}

  float aspect_ratio() const {return float(m_w) / float(m_h);}

  std::uint32_t n_swapchain_images() const {return m_sw_images.size();}

	VkImageView sw_view(std::uint32_t idx) const {return m_sw_views[idx];}
	VkImage sw_image(std::uint32_t idx) const {return m_sw_images[idx];}

	VkImageView depth_view() const {return m_depth_view;}
	VkImage depth_image() const {return m_depth_image;}
	

	const VkFormat& color_format() const {return m_sw_format.format;}
	const VkFormat& depth_format() const {return m_depth_format;}

	~DisplayInstance() {destroy();}

  template<typename InstanceInfo = DefaultInstanceInfo>
  struct DisplayInstanceInfo : public InstanceInfo
  {
    SDL_Window* m_wind;

    DisplayInstanceInfo(Display& disp, InstanceInfo&& ii = {}) : InstanceInfo(ii), m_wind(disp.window()) {}

    ExtensionsLayers inst_exts_lays() 
    {
      auto el = InstanceInfo::inst_exts_lays();

      Uint32 n_exts;
      const char * const * exts = SDL_Vulkan_GetInstanceExtensions(&n_exts);

      for(auto elem = exts, end = exts + n_exts; elem != end; ++elem)
      {
        if(!std::any_of(el.exts.begin(), el.exts.end(), [elem](const std::string& s) {return s == *elem;}))
          el.exts.emplace_back(*elem);
      }
			return el;
    }

		auto dev_exts() {
      auto vec = InstanceInfo::dev_exts();
      vec.push_back("VK_KHR_swapchain");
      return vec;
    }

    VkImageUsageFlags depth_image_additional_usage() const {return 0;}
  };

	template<typename InstanceInfo>
	DisplayInstance(Display& disp, InstanceInfo&& ii) : Instance(DelayedDeviceCreation(), ii)
	{
		create_surf(disp.m_wind);

		create_device(ii, m_surf);
		get_present_queue();

		create_sw();
		create_sw_views();
		find_depth_format();

    m_depth_image_usage = ii.depth_image_additional_usage() | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		create_depth_image();
		create_semaphores();
		create_fence();
	}

  DisplayInstance(Display& disp) : DisplayInstance(disp, DisplayInstanceInfo(disp, DefaultInstanceInfo())) {}

	void destroy();

	void create_sw();

	VkSemaphore image_available_semaphore() const {return m_image_avail_sem;}

	bool acquire_next_image(std::uint32_t* index)
	{
		auto res = vkAcquireNextImageKHR(m_dev, m_sw, 0, m_image_avail_sem, VK_NULL_HANDLE, index);
		
    switch(res)
    {
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
      return true;
    case VK_TIMEOUT:
    case VK_NOT_READY:
      return false;
    default:
      vk_check(res);
      return true;
    }
	}

	void submit_render_present(VkCommandBuffer cmd_buf, uint32_t img_idx);

	bool render_done() const
	{
		auto res = vkWaitForFences(m_dev, 1, &m_render_done_fence, VK_TRUE, 0);

		if(res > 0) return false;
		
		vk_check(res);

		return true;
	}

  void begin_rendering(VkCommandBuffer buffer, std::uint32_t img_idx, bool secondary_contents = false, VkClearColorValue clear_color_value = {{0.25f, 0.25f, 0.25f, 0.0f}});
  void end_rendering(VkCommandBuffer cmd_buf, std::uint32_t img_idx);

  void set_dynamic_viewport_scissor(VkCommandBuffer cmd_buf)
  {
    VkRect2D sc{{0, 0}, {w(), h()}};
    VkViewport vp{
      0.0,
      0.0,
      float(w()),
      float(h()),
      0.0f,
      1.0f
    };

    vkCmdSetViewport(cmd_buf, 0, 1, &vp);
    vkCmdSetScissor(cmd_buf, 0, 1, &sc);
  }

private:

  void get_present_queue();

	void find_sw_info();

	void create_surf(SDL_Window* wind);

	void create_sw_views();
	void create_depth_image();

	void find_depth_format();
	void create_transfer_pool();


	/** Takes needed extensions and layers as arg and adds extensions required by layers and by the SDL_Window of display */
	void fill_exts_lays(ExtensionsLayers& el);

	void create_semaphores();
	void create_fence();

  void resize();

  void destroy_swapchain();
  void destroy_depth_image();
};

}


#endif
