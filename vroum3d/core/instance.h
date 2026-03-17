#ifndef VROUM3D_CORE_INSTANCE_H_INCLUDED
#define VROUM3D_CORE_INSTANCE_H_INCLUDED

#include "../utility.h"
#include "../debug.h"
#include "display.hpp"
#include "allocator.h"

#include <SDL2/SDL_video.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <SDL2/SDL_vulkan.h>

#include <array>
#include <vector>
#include <cstring>
#include <stdexcept>


namespace Vroum3d::Core
{

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
  Instance(const InstanceInfo& ii = {})
  {
		create_instance(ii.inst_exts_lays());

		choose_pdev();
		create_device(ii.dev_exts());

		create_transfer_pool();

	}
protected:
	/** Create Instance with VkInstance only, to allow surface creation before device creation (for present queue) */
	template<typename InstanceInfo = DefaultInstanceInfo>
  Instance(DelayedDeviceCreation, const InstanceInfo& ii = {})
	{
		create_instance(ii.inst_exts_lays());
	}

	template<typename InstanceInfo = DefaultInstanceInfo>
	void create_device(const InstanceInfo& ii = {}, VkSurfaceKHR surf = VK_NULL_HANDLE)
	{
		choose_pdev();
		create_device(ii.dev_exts(), surf);

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
private:


	void create_instance(ExtensionsLayers&& el);

	void choose_pdev();
	void create_device(const std::vector<std::string>& exts, VkSurfaceKHR surf = VK_NULL_HANDLE);
	void create_transfer_pool();
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
	std::uint32_t m_w, m_h;

public:

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

  template<typename InstanceInfo>
  struct DisplayInstanceInfo
  {
    const InstanceInfo& m_ii;
    SDL_Window* m_wind;

    DisplayInstanceInfo(Display& disp, const InstanceInfo& ii) : m_ii(ii), m_wind(disp.window()) {}

    ExtensionsLayers inst_exts_lays() const
    {
      auto el = m_ii.inst_exts_lays();

      auto wind_lays = wrap_enumerate<SDL_Vulkan_GetInstanceExtensions>(m_wind);
      for(auto elem : wind_lays)
      {
        if(!std::any_of(el.exts.begin(), el.exts.end(), [elem](const std::string& s) {return s == elem;}))
          el.exts.emplace_back(elem);
      }
			return el;
    }

		auto dev_exts() const {
      auto vec = m_ii.dev_exts();
      vec.push_back("VK_KHR_swapchain");
      return vec;
    }
  };

	template<typename InstanceInfo = DefaultInstanceInfo>
	DisplayInstance(Display& disp, const InstanceInfo& ii = {}) : Instance(DelayedDeviceCreation(), DisplayInstanceInfo(disp, ii))
	{
		create_surf(disp.m_wind);

		create_device(DisplayInstanceInfo(disp, ii), m_surf);
		get_present_queue();

		create_sw();
		create_sw_views();
		find_depth_format();
		create_depth_image();
		create_semaphores();
		create_fence();
	}

	void destroy();

	void create_sw();

	void quick_submit(VkCommandBuffer cmd_buf);

	VkSemaphore image_available_semaphore() const {return m_image_avail_sem;}

	bool acquire_next_image(std::uint32_t* index)
	{
		auto res = vkAcquireNextImageKHR(m_dev, m_sw, 0, m_image_avail_sem, VK_NULL_HANDLE, index);
		
		if(res > 0) return false;
		
		vk_check(res);

		return true;
	}

	void submit_render_present(VkCommandBuffer cmd_buf, uint32_t img_idx);

	bool render_done() const
	{
		auto res = vkWaitForFences(m_dev, 1, &m_render_done_fence, VK_TRUE, 0);

		if(res > 0) return false;
		
		vk_check(res);

		return true;
	}

  void begin_rendering(VkCommandBuffer buffer, std::uint32_t img_idx, bool secondary_contents = false);
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

	void create_semaphores();
	void create_fence();
};

struct DefaultInstanceInfo
{

	template<bool dbg = false>
	struct DefaultInstanceExtensions {
		static constexpr std::array<const char*, 0> value = {};
	};

	template<bool dbg = false>
	struct DefaultInstanceLayers {
		static constexpr std::array value = [](){
			if constexpr (dbg) return std::array<const char*, 1>{"VK_LAYER_KHRONOS_validation"};
			else return std::array<const char*, 0>{};

		}();
	};

	DisplayInstance::ExtensionsLayers inst_exts_lays() const
	{	
		constexpr auto extarray = DefaultInstanceExtensions<debug>::value;
		constexpr auto layarray = DefaultInstanceLayers<debug>::value;

		return {{extarray.begin(), extarray.end()} ,
			{layarray.begin(), layarray.end()}};
	}

	std::vector<std::string> dev_exts() const {return {};}

};

}


#endif
