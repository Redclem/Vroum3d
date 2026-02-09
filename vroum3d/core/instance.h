#ifndef VROUM3D_CORE_INSTANCE_H_INCLUDED
#define VROUM3D_CORE_INSTANCE_H_INCLUDED

#include "../utility.h"
#include "../debug.h"
#include "display.hpp"

#include <SDL2/SDL_video.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <SDL2/SDL_vulkan.h>

#include <array>
#include <vector>


namespace Vroum3d::Core
{

template<bool enable = false>
class InstanceDebugData {
	void destroy(VkInstance) {}
};

struct DefaultInstanceInfo;

template<>
class InstanceDebugData<true>
{
public:
	VkHandle<VkDebugUtilsMessengerEXT> m_dbg_messenger;
html
	void destroy(VkInstance inst)
	{

		m_dbg_messenger.destroy_with([&](auto msg){
			PFN_vkDestroyDebugUtilsMessengerEXT fn = reinterpret_cast<decltype(fn)>(vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"));
			if(fn) fn(inst, msg, nullptr);
		});
	}
};

class Instance : public AssignDestroy<Instance>, private InstanceDebugData<debug>
{
	VkHandle<VkInstance> m_inst;
	VkPhysicalDevice m_pdev;
	VkHandle<VkDevice> m_dev;
	VkHandle<VkSurfaceKHR> m_surf;
	std::uint32_t m_w, m_h;
	VkHandle<VkSwapchainKHR> m_sw;
	VkSurfaceFormatKHR m_sw_format;
	std::uint32_t m_ti, m_gi;
	VkQueue m_tq, m_pq, m_gq;
	std::vector<VkHandle<VkImageView>> m_sw_views;
	VkHandle<VkImage> m_depth_image;
	VkHandle<VkImageView> m_depth_view;
	VkHandle<VkDeviceMemory> m_depth_mem;
	VkHandle<VkCommandPool> m_transfer_pool;
	VkFormat m_depth_format;
	VkHandle<VkSemaphore> m_image_avail_sem;

	struct
	{
		VkQueue tranfer, present, graphics;
	} m_queues;

	using debug_data_t = InstanceDebugData<debug>;

public:

	auto w() const {return m_w;}
	auto h() const {return m_h;}

	VkImageView sw_view(std::uint32_t idx) const {return m_sw_views[idx].get();}

	VkImageView depth_view() const {return m_depth_view;}

	const VkFormat& color_format() const {return m_sw_format.format;}
	const VkFormat& depth_format() const {return m_depth_format;}

	~Instance() {destroy();}

	struct ExtensionsLayers
	{
		std::vector<std::string> exts, lays;
	};

	template<typename InstanceInfo = DefaultInstanceInfo>
	Instance(Display& disp, const InstanceInfo& ii = {})
	{
		ExtensionsLayers el = ii.inst_exts_lays();
		fill_exts_lays(el, disp.m_wind);

		create_instance(el);
		create_surf(disp.m_wind);

		choose_pdev();
		create_device(ii.dev_exts());

		create_sw();
		create_sw_views();
		find_depth_format();
		create_depth_image();

		create_transfer_pool();
		create_semaphores();
	}

	auto& queues() const {return m_queues;}

	void destroy();

	void create_sw();

	VkDevice device() const {return m_dev;}
	VkPhysicalDevice pdev() const {return m_pdev;}
	VkCommandPool transfer_pool() const {return m_transfer_pool;}

	void quick_submit(VkCommandBuffer cmd_buf);

	VkSemaphore image_available_semaphore() const {return m_image_avail_sem;}

	bool acquire_next_image(std::uint32_t* index)
	{
		auto res = vkAcquireNextImageKHR(m_dev, m_sw, 0, m_image_avail_sem, VK_NULL_HANDLE, index);
		
		if(res == VK_TIMEOUT) return false;
		
		vk_check(res);

		return true;
	}

private:

	void find_sw_info();

	void create_instance(const ExtensionsLayers& el);

	void choose_pdev();
	void create_device(const std::vector<std::string>& exts);

	void create_surf(SDL_Window* wind);

	void create_sw_views();
	void create_depth_image();

	void find_depth_format();
	void create_transfer_pool();

	/** Takes needed extensions and layers as arg and adds extensions required by layers and by the SDL_Window of display */
	void fill_exts_lays(ExtensionsLayers& el, SDL_Window* wind);

	void create_semaphores();
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

	Instance::ExtensionsLayers inst_exts_lays() const
	{	
		constexpr auto extarray = DefaultInstanceExtensions<debug>::value;
		constexpr auto layarray = DefaultInstanceLayers<debug>::value;

		return {{extarray.begin(), extarray.end()} ,
			{layarray.begin(), layarray.end()}};
	}

	std::vector<std::string> dev_exts() const {return {"VK_KHR_swapchain"};}

};

}


#endif
