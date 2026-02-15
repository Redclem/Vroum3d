#include "instance.h"
#include "vkutil.h"

#include "../version.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include <iostream>
#include <algorithm>
#include <cstring>

using namespace Vroum3d::Core;

void Instance::destroy()
{
	vkDeviceWaitIdle(m_dev);

	m_render_done_fence.destroy_with([&](auto fnc){vkDestroyFence(m_dev, fnc, nullptr);});

	m_image_avail_sem.destroy_with([&](auto sem){vkDestroySemaphore(m_dev, sem, nullptr);});
	m_render_finished_semaphore.destroy_with([&](auto sem){vkDestroySemaphore(m_dev, sem, nullptr);});

	m_transfer_pool.destroy_with([&](auto pl){vkDestroyCommandPool(m_dev, pl, nullptr);});

	m_depth_view.destroy_with([&](auto dv){vkDestroyImageView(m_dev, dv, nullptr);});
	m_depth_image.destroy_with([&](auto di){vkDestroyImage(m_dev, di, nullptr);});
	m_depth_mem.destroy_with([&](auto mem) {vkFreeMemory(m_dev, mem, nullptr);});

	for(auto& elem : m_sw_views)
		if(elem != VK_NULL_HANDLE)
			vkDestroyImageView(m_dev, elem, nullptr);

	m_sw_views.clear();

	m_sw.destroy_with([&](auto sw) {vkDestroySwapchainKHR(m_dev, sw, nullptr);});
	m_dev.destroy_with([&](auto dev) {vkDestroyDevice(dev, nullptr);});

	debug_data_t::destroy(m_inst);

	m_surf.destroy_with([&](auto surf){vkDestroySurfaceKHR(m_inst, surf, nullptr);});
	m_inst.destroy_with([&](auto inst){vkDestroyInstance(inst, nullptr);});

}

void Instance::create_instance(const ExtensionsLayers& el)
{
	VkApplicationInfo appi = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr,
		"Vroum3d App",
		VK_MAKE_VERSION(0, 0, 0),
		"Vroum3d",
		Version::eng_ver,
		Version::vk_api_ver
	};

	std::vector<const char*> lays;
	lays.reserve(el.lays.size());
	for(const auto& elem : el.lays)
		lays.emplace_back(elem.c_str());

	std::vector<const char*> exts;
	exts.reserve(el.exts.size());
	for(const auto& elem : el.exts)
		exts.emplace_back(elem.c_str());

	VkInstanceCreateInfo nfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		nullptr,
		0,
		&appi,
		std::uint32_t(lays.size()),
		lays.data(),
		std::uint32_t(exts.size()),
		exts.data()
	};

	if constexpr(debug)
	{
		PFN_vkDebugUtilsMessengerCallbackEXT callback = [](
			VkDebugUtilsMessageSeverityFlagBitsEXT sever,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* cbd,
			void*
		       ) -> VkBool32 {
			std::cout << "Vulkan Message:\n";
			std::cout << "Severity:" << string_VkDebugUtilsMessageSeverityFlagsEXT(sever) << '\n';
			std::cout << "Type:" << string_VkDebugUtilsMessageTypeFlagsEXT(type) << '\n';
			std::cout << cbd->pMessage << "\n\n\n";
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

		nfo.pNext = &dbi;

		vk_check(vkCreateInstance(&nfo, nullptr, &m_inst));

		PFN_vkCreateDebugUtilsMessengerEXT fn = reinterpret_cast<decltype(fn)>(vkGetInstanceProcAddr(m_inst, "vkCreateDebugUtilsMessengerEXT"));

		check(fn);

		vk_check(fn(m_inst, &dbi, nullptr, &m_dbg_messenger));
	}
	else
		vk_check(vkCreateInstance(&nfo, nullptr, &m_inst));
}

void Instance::choose_pdev()
{
	std::vector<VkPhysicalDevice> pdevs = wrap_enumerate<vkEnumeratePhysicalDevices>(m_inst);

	check(pdevs.size());

	VkPhysicalDevice best_pdev(VK_NULL_HANDLE);
	int best_score(-1);

	for(auto pdev : pdevs)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(pdev, &props);

		int score = 0;

		switch(props.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score = 2;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score = 1;
			break;
		default:
			break;
		}

		if(score > best_score)
		{
			best_pdev = pdev;
			best_score = score;
		}
	}

	m_pdev = best_pdev;
}

void Instance::fill_exts_lays(ExtensionsLayers& el, SDL_Window* wind)
{

	std::vector<VkLayerProperties> lprops =
		wrap_enumerate<vkEnumerateInstanceLayerProperties>();

	auto proc_lay = [&](const char* elem){

		if(!std::any_of(lprops.begin(), lprops.end(), [elem](const VkLayerProperties& lp) {return std::strcmp(elem, lp.layerName) == 0;}))
		{
			log("Missing extension ", elem);
			throw std::runtime_error("Missing extension");
		}

		auto lay_exts = wrap_enumerate<vkEnumerateInstanceExtensionProperties>(elem);

		for(const auto& ext : lay_exts)
			if(!std::any_of(el.exts.begin(), el.exts.end(), [ext](const std::string& s) {return s == ext.extensionName;}))
				el.exts.emplace_back(ext.extensionName);
	};

	for(auto elem : el.lays)
		proc_lay(elem.c_str());

	auto wind_lays = wrap_enumerate<SDL_Vulkan_GetInstanceExtensions>(wind);
	for(auto elem : wind_lays)
	{
		if(!std::any_of(el.exts.begin(), el.exts.end(), [elem](const std::string& s) {return s == elem;}))
			el.exts.emplace_back(elem);
	}
}

void Instance::create_device(const std::vector<std::string>& exts)
{
	auto queues = wrap_enumerate<vkGetPhysicalDeviceQueueFamilyProperties>(m_pdev);

	int it(-1), ip(-1), ig(-1);
	
	for(int idx = 0; const auto& elem : queues)
	{
		if(elem.queueFlags & VK_QUEUE_TRANSFER_BIT && it == -1)
			it = idx;
		if(elem.queueFlags & VK_QUEUE_GRAPHICS_BIT && ig == -1)
			ig = idx;
		
		VkBool32 supp;
		vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(m_pdev, idx, m_surf, &supp));
	
		if(supp && ip == -1)
			ip = idx;

		++idx;
	}

	if(it == -1 || ip == -1 || ig == -1)
		throw std::runtime_error("missing queue");

	std::vector<VkDeviceQueueCreateInfo> dqis;

	{
		std::vector<std::uint32_t> idxes;
		auto ens_idx = [&](std::uint32_t idx)
			{
				if(idxes.end() == std::find(idxes.begin(), idxes.end(), idx))
					idxes.push_back(idx);
			};

		ens_idx(it);
		ens_idx(ip);
		ens_idx(ig);

		dqis.resize(idxes.size());

		auto iteri = idxes.begin();
		auto iterq = dqis.begin();
		float prio = 1.0f;

		for(;iteri != idxes.end(); iteri++, iterq++)
		{
			*iterq = {
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,
				0,
				*iteri,
				1,
				&prio
			};
		}
	}

	std::vector<const char*> exts_c_str;
	exts_c_str.reserve(exts.size());

	for(auto& elem : exts)
		exts_c_str.push_back(elem.c_str());

	VkDeviceCreateInfo di{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		std::uint32_t(dqis.size()),
		dqis.data(),
		0,
		nullptr,
		std::uint32_t(exts.size()),
		exts_c_str.data(),
		nullptr
	};

	VkPhysicalDeviceVulkan13Features vk13feats{};
	vk13feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vk13feats.dynamicRendering = VK_TRUE;

	di.pNext = &vk13feats;

	vk_check(vkCreateDevice(m_pdev, &di, nullptr, &m_dev));

	vkGetDeviceQueue(m_dev, it, 0, &m_tq);
	vkGetDeviceQueue(m_dev, ip, 0, &m_pq);
	vkGetDeviceQueue(m_dev, ig, 0, &m_gq);
}

void Instance::create_surf(SDL_Window* wind)
{
	check(SDL_Vulkan_CreateSurface(wind, m_inst, &m_surf) == SDL_TRUE);
	int w, h;
	SDL_GetWindowSize(wind, &w, &h);
	m_w = w, m_h = h;
}

void Instance::create_sw()
{
	find_sw_info();

	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pdev, m_surf, &caps);

	VkSwapchainCreateInfoKHR swi{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,
		0,
		m_surf,
		caps.minImageCount,
		m_sw_format.format,
		m_sw_format.colorSpace,
		{m_w, m_h},
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_TRUE,
		VK_NULL_HANDLE
	};

	vk_check(vkCreateSwapchainKHR(m_dev, &swi, nullptr, &m_sw));
}

void Instance::find_sw_info()
{
	auto forms = wrap_enumerate<vkGetPhysicalDeviceSurfaceFormatsKHR>(m_pdev, m_surf);

	for(auto& elem : forms)
	{
		if(elem.format == VK_FORMAT_B8G8R8_SRGB)
		{
			m_sw_format = elem;
			return;
		}
	}

	m_sw_format = forms[0];
}

void Instance::create_sw_views()
{
	auto images = wrap_enumerate<vkGetSwapchainImagesKHR>(m_dev, m_sw);

	m_sw_views.reserve(images.size());

	for(auto elem : images)
	{
		VkImageViewCreateInfo vi{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			elem,
			VK_IMAGE_VIEW_TYPE_2D,
			m_sw_format.format,
			vkutil::components_id,
			vkutil::color_subres_plain
		};

		m_sw_views.emplace_back(VK_NULL_HANDLE);
		vk_check(vkCreateImageView(m_dev, &vi, nullptr, &m_sw_views.back()));
	}
}

void Instance::create_depth_image()
{

	VkImageCreateInfo ii{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		m_depth_format,
		{m_w, m_h, 1},
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	vk_check(vkCreateImage(m_dev, &ii, nullptr, &m_depth_image));

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(m_dev, m_depth_image, &mr);

	VkMemoryAllocateInfo mai{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(m_pdev, mr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	vk_check(vkAllocateMemory(m_dev, &mai, nullptr, &m_depth_mem));
	vk_check(vkBindImageMemory(m_dev, m_depth_image, m_depth_mem, 0));

	VkImageViewCreateInfo vi{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		m_depth_image,
		VK_IMAGE_VIEW_TYPE_2D,
		m_depth_format,
		vkutil::components_id,
		{
			VK_IMAGE_ASPECT_DEPTH_BIT,
			0,
			1,
			0,
			1
		}
	};

	vk_check(vkCreateImageView(m_dev, &vi, nullptr, &m_depth_view));

}

void Instance::find_depth_format()
{
	auto format_depth_supp = [&](VkFormat f)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_pdev, f, &props);

		return props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	};

	m_depth_format = format_depth_supp(VK_FORMAT_D32_SFLOAT) ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_X8_D24_UNORM_PACK32;
}

void Instance::create_transfer_pool()
{
	VkCommandPoolCreateInfo pi{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		m_ti
	};

	vk_check(vkCreateCommandPool(m_dev, &pi, nullptr, &m_transfer_pool));
}

void Instance::create_semaphores()
{
	VkSemaphoreCreateInfo si{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};

	vk_check(vkCreateSemaphore(m_dev, &si, nullptr, &m_image_avail_sem));
	vk_check(vkCreateSemaphore(m_dev, &si, nullptr, &m_render_finished_semaphore));
}

void Instance::submit_render_present(VkCommandBuffer cmd_buf, uint32_t idx)
{	
	vk_check(vkResetFences(m_dev, 1, &m_render_done_fence));

	VkCommandBufferSubmitInfo cbi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		nullptr,
		cmd_buf,
		0
	};

	VkSemaphoreSubmitInfo ssiw{
		VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		nullptr,
		m_image_avail_sem,
		0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0
	};

	VkSemaphoreSubmitInfo ssis{
		VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		nullptr,
		m_render_finished_semaphore,
		0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0
	};

	VkSubmitInfo2 si{
		VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		nullptr,
		0,
		1,
		&ssiw,
		1,
		&cbi,
		1,
		&ssis
	};

	vk_check(vkQueueSubmit2(m_gq, 1, &si, m_render_done_fence));

	VkPresentInfoKHR pi{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&m_render_finished_semaphore,
		1,
		&m_sw,
		&idx,
		nullptr
	};

	vk_check(vkQueuePresentKHR(m_pq, &pi));
}

void Instance::create_fence()
{
	VkFenceCreateInfo fi{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		VK_FENCE_CREATE_SIGNALED_BIT
	};

	vk_check(vkCreateFence(m_dev, &fi, nullptr, &m_render_done_fence));
}
