#ifndef VROUM3D_GUI_BASE_H_INCLUDED
#define VROUM3D_GUI_BASE_H_INCLUDED

#include "../core/instance.h"
#include "element.h"

#include <map>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Gui
{

class Element;

using namespace Core;

class Base : public AssignDestroy<Base>
{
public:

	using init_elements_t = std::vector<Element*>;


private:
	struct Texture
	{
		VkHandle<VkImage> img;
		VkHandle<VkImageView> view;

		std::uint32_t w, h;
	};
	
	using texture_container_t = std::map<std::string, Texture>;
	bool rgb_supported();


	void set_root_elem(Element* elem) {
		m_root_elem = elem;
	}

	Element* m_first_elem = nullptr, *m_root_elem = nullptr;

	const Instance* m_instance;
	VkDevice m_device;

	VkHandle<VkBuffer> m_buffer;
	VkHandle<VkDeviceMemory> m_mem;
	texture_container_t m_textures;
	bool m_rgb;

public:

	~Base() {
		destroy();
	}

	void destroy();

	Base(const Instance& inst) : m_instance(&inst), m_device(inst.device()) {}

	VkDevice device() const {return m_device;}

	void init();

	void register_element(Element* elem)
	{
		elem->m_next_element = m_first_elem;
		m_first_elem = elem;
	}
};

}

#endif
