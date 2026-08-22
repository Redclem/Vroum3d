#include "instance.h"
#include "objects.h"
#include "vkutil.h"

#include "../version.h"

#include <limits>
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <stdexcept>

#include <algorithm>
#include <cstring>
#include <string>

using namespace Vroum3d::Core;

void DisplayInstance::destroy()
{
  if(m_dev != VK_NULL_HANDLE)
	  vkDeviceWaitIdle(m_dev);

	for(auto& frame_sync : m_frame_sync)
	{
		frame_sync.image_avail_sem.destroy_with([&](VkSemaphore sem){vkDestroySemaphore(device(), sem, nullptr);});
		frame_sync.render_done_sem.destroy_with([&](VkSemaphore sem){vkDestroySemaphore(device(), sem, nullptr);});
		frame_sync.render_done_fence.destroy_with([&](VkFence fnc){vkDestroyFence(device(), fnc, nullptr);});
	}

	m_depth_view.destroy_with([&](auto dv){vkDestroyImageView(m_dev, dv, nullptr);});
	m_depth_image.destroy_with([&](auto di){vkDestroyImage(m_dev, di, nullptr);});

  free(m_depth_mem);

	for(auto& elem : m_sw_views)
		if(elem != VK_NULL_HANDLE)
			vkDestroyImageView(m_dev, elem, nullptr);

	m_sw_views.clear();

	m_sw.destroy_with([&](auto sw) {vkDestroySwapchainKHR(m_dev, sw, nullptr);});


	m_surf.destroy_with([&](auto surf){vkDestroySurfaceKHR(m_inst, surf, nullptr);});

  Instance::destroy();
}

void Instance::destroy()
{
  if(m_dev != VK_NULL_HANDLE)
	  vkDeviceWaitIdle(m_dev);

	m_transfer_pool.destroy_with([&](auto pl){vkDestroyCommandPool(m_dev, pl, nullptr);});
	Allocator::destroy();
	m_dev.destroy_with([&](auto dev) {vkDestroyDevice(dev, nullptr);});
	debug_data_t::destroy(m_inst);
	m_inst.destroy_with([&](auto inst){vkDestroyInstance(inst, nullptr);});
}

void Instance::create_instance(ExtensionsLayers&& el, void* instance_pnext)
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
		instance_pnext,
		0,
		&appi,
		std::uint32_t(lays.size()),
		lays.data(),
		std::uint32_t(exts.size()),
		exts.data()
	};
		

  vk_check(vkCreateInstance(&nfo, nullptr, &m_inst));
}

void InstanceDebugData<true>::create_dbg_mesg(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT& ci)
{
		PFN_vkCreateDebugUtilsMessengerEXT fn = reinterpret_cast<decltype(fn)>(vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT"));

		check(fn);

		vk_check(fn(inst, &ci, nullptr, &m_dbg_messenger));
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

void Instance::create_device(const std::vector<std::string>& exts, VkSurfaceKHR surf, void* device_pnext)
{
	auto queues = wrap_enumerate<vkGetPhysicalDeviceQueueFamilyProperties>(m_pdev);

	int it(-1), ip(-1), ig(-1);
  {
    int idx = 0;
    for(const auto& elem : queues)
    {
      if(elem.queueFlags & VK_QUEUE_TRANSFER_BIT && it == -1)
        it = idx;
      if(elem.queueFlags & VK_QUEUE_GRAPHICS_BIT && ig == -1)
        ig = idx;
      
			if(surf != VK_NULL_HANDLE)
			{
				VkBool32 supp;
				vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(m_pdev, idx, surf, &supp));
			
				if(supp && ip == -1)
					ip = idx;
			}

      ++idx;
    }
  }

	if(it == -1 || ig == -1)
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
		if(surf != VK_NULL_HANDLE)
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

	VkPhysicalDeviceVulkan13Features vk13feats{};
	vk13feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  vk13feats.pNext = device_pnext;
	vk13feats.dynamicRendering = VK_TRUE;
	vk13feats.synchronization2 = VK_TRUE;

	VkDeviceCreateInfo di{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		&vk13feats,
		0,
		std::uint32_t(dqis.size()),
		dqis.data(),
0,
		nullptr,
		std::uint32_t(exts.size()),
		exts_c_str.data(),
		nullptr
	};

	vk_check(vkCreateDevice(m_pdev, &di, nullptr, &m_dev));

	vkGetDeviceQueue(m_dev, it, 0, &m_tq);
	vkGetDeviceQueue(m_dev, ig, 0, &m_gq);

	m_ti = it;
	m_gi = ig;

  Allocator::init(m_dev, m_pdev);
}

void DisplayInstance::create_surf(SDL_Window* wind)
{
	check(SDL_Vulkan_CreateSurface(wind, m_inst, nullptr, &m_surf));
	int w, h;
	SDL_GetWindowSize(wind, &w, &h);
	m_w = w, m_h = h;
}

void DisplayInstance::create_sw()
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
		caps.currentExtent,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		m_sw_pres_mode,
		VK_TRUE,
		VK_NULL_HANDLE
	};

	vk_check(vkCreateSwapchainKHR(m_dev, &swi, nullptr, &m_sw));

  m_w = caps.currentExtent.width;
  m_h = caps.currentExtent.height;
}

void DisplayInstance::find_sw_info()
{
	auto forms = wrap_enumerate<vkGetPhysicalDeviceSurfaceFormatsKHR>(m_pdev, m_surf);

	m_sw_format = forms[0];

	for(auto& elem : forms)
	{
		if(elem.format == VK_FORMAT_B8G8R8_SRGB)
		{
			m_sw_format = elem;
			break;
		}
	}

	auto modes = wrap_enumerate<vkGetPhysicalDeviceSurfacePresentModesKHR>(m_pdev, m_surf);

	m_sw_pres_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (auto& elem : modes)
	{
		if (elem == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			m_sw_pres_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
}

void DisplayInstance::create_sw_views()
{
	m_sw_images = wrap_enumerate<vkGetSwapchainImagesKHR>(m_dev, m_sw);

	m_sw_views.reserve(m_sw_images.size());

	for(auto elem : m_sw_images)
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

void DisplayInstance::create_depth_image()
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
		m_depth_image_usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	vk_check(vkCreateImage(m_dev, &ii, nullptr, &m_depth_image));

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(m_dev, m_depth_image, &mr);

	/*VkMemoryAllocateInfo mai{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		mr.size,
		vkutil::find_mem_index(m_pdev, mr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};*/

  m_depth_mem = allocate(vkutil::find_mem_index(m_pdev, mr, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), mr.size, mr.alignment);

	vk_check(vkBindImageMemory(m_dev, m_depth_image, m_depth_mem.memory(), m_depth_mem.offset()));

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

void DisplayInstance::find_depth_format()
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

void DisplayInstance::submit_render_present(VkCommandBuffer cmd_buf)
{	
	auto& sync = m_frame_sync[m_next_frame];

	vk_check(vkResetFences(m_dev, 1, &sync.render_done_fence));

	VkCommandBufferSubmitInfo cbi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		nullptr,
		cmd_buf,
		0
	};

	VkSemaphoreSubmitInfo ssiw{
		VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		nullptr,
		sync.image_avail_sem,
		0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0
	};

	VkSemaphoreSubmitInfo ssis{
		VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		nullptr,
		sync.render_done_sem,
		0,
		VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
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

	vk_check(vkQueueSubmit2(m_gq, 1, &si, sync.render_done_fence));

	VkPresentInfoKHR pi{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&sync.render_done_sem,
		1,
		&m_sw,
		&sync.image_index,
		nullptr
	};

  auto res = vkQueuePresentKHR(m_pq, &pi);

  switch(res)
  {
  case VK_SUBOPTIMAL_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
    resize();
    break;
  default:
    vk_check(res);
  }
}

void Instance::quick_submit(VkCommandBuffer cmd_buf)
{
  VkFence fnc;

	VkFenceCreateInfo fi{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		0
	};

	vk_check(vkCreateFence(m_dev, &fi, nullptr, &fnc));

	VkCommandBufferSubmitInfo cbi{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		nullptr,
		cmd_buf,
		0
	};

	VkSubmitInfo2 si{
		VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		nullptr,
		0,
    0,
    nullptr,
		1,
		&cbi,
    0,
    nullptr
	};

	vk_check(vkQueueSubmit2(m_gq, 1, &si, fnc));

  vk_check(vkWaitForFences(m_dev, 1, &fnc, VK_TRUE, ~std::uint64_t(0)));

  vkDestroyFence(m_dev, fnc, nullptr);
}

void DisplayInstance::begin_rendering(VkCommandBuffer cmd_buf, bool secondary_contents, VkClearColorValue clear_color_value)
{
	std::array<VkImageMemoryBarrier2, 2> barriers = {{
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		0,
		0,
		sw_image(next_frame_index()),
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	},
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		0,
		0,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		0,
		0,
		depth_image(),
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	}
	}};

	VkDependencyInfo di{
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		barriers.size(),
		barriers.data()
	};

	vkCmdPipelineBarrier2(cmd_buf, &di);

	VkRenderingAttachmentInfo
	catt{
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		nullptr,
		sw_view(next_frame_index()),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_RESOLVE_MODE_NONE,
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		{.color = clear_color_value}
	};

	VkRenderingAttachmentInfo
	datt{
		VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		nullptr,
		depth_view(),
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
		VK_RESOLVE_MODE_NONE,
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		{.depthStencil = {1.0, 1}}
	};
	
	VkRenderingInfo ri{
		VK_STRUCTURE_TYPE_RENDERING_INFO,
		nullptr,
		secondary_contents ? VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT : VkRenderingFlagBits(0),
		{{0, 0}, {w(), h()}},
		1,
		0,
		1,
		&catt,
		&datt,
		nullptr
	};

	vkCmdBeginRendering(cmd_buf, &ri);
}

void DisplayInstance::end_rendering(VkCommandBuffer cmd_buf)
{
	vkCmdEndRendering(cmd_buf);

	VkImageMemoryBarrier2 bar = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		nullptr,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		0,
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		0,
		0,
		sw_image(next_frame_index()),
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};

	VkDependencyInfo di{
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&bar
	};

	vkCmdPipelineBarrier2(cmd_buf, &di);
}

void DisplayInstance::get_present_queue()
{
	std::uint32_t n_queues;
	vkGetPhysicalDeviceQueueFamilyProperties(pdev(), &n_queues, nullptr);

	int ip(-1);
  {
    for(int idx = 0; idx != int(n_queues); ++idx)
    {
			VkBool32 supp;
			vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(m_pdev, idx, m_surf, &supp));
		
			if(supp && ip == -1)
				ip = idx;
    }
  }

	if(ip == -1)
		throw std::runtime_error("No present support!");

	vkGetDeviceQueue(device(), ip, 0, &m_pq);
}

void DisplayInstance::resize()
{
  m_resized = true;

  VkSurfaceCapabilitiesKHR surf_cap;
  vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev(), m_surf, &surf_cap));

  vk_check(vkQueueWaitIdle(m_pq));

  destroy_swapchain();
  destroy_depth_image();

  create_sw();
  create_sw_views();
  create_depth_image();
}

void DisplayInstance::destroy_swapchain()
{
  for(auto& elem : m_sw_views)
    vkDestroyImageView(device(), elem, nullptr);

  m_sw_views.clear();
  m_sw_images.clear();

  m_sw.destroy_with([&]{vkDestroySwapchainKHR(device(), m_sw, nullptr);});
}

void DisplayInstance::destroy_depth_image()
{
  m_depth_view.destroy_with([&]{vkDestroyImageView(device(), m_depth_view, nullptr);});
  m_depth_image.destroy_with([&]{vkDestroyImage(device(), m_depth_image, nullptr);});

  free(m_depth_mem);
}

void DisplayInstance::create_frame_sync()
{
	VkSemaphoreCreateInfo si{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};

	VkFenceCreateInfo fi{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		VK_FENCE_CREATE_SIGNALED_BIT
	};

	for(auto& elem : m_frame_sync)
	{
		vk_check(vkCreateSemaphore(device(), &si, nullptr, &elem.image_avail_sem));
		vk_check(vkCreateSemaphore(device(), &si, nullptr, &elem.render_done_sem));

		vk_check(vkCreateFence(device(), &fi, nullptr, &elem.render_done_fence));
	}
}
