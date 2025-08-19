#ifndef VROUM3D_GUI_BASE_H_INCLUDED
#define VROUM3D_GUI_BASE_H_INCLUDED

#include "../core/instance.h"

#include <map>
#include <vulkan/vulkan_core.h>

namespace Vroum3d::Gui
{

class Element;

using namespace Core;

class Base : public AssignDestroy<Base>
{
	friend class Element;

	struct Texture
	{
		VkImage img;
		VkImageView view;

		std::uint32_t w, h;
		bool alpha = false;
	};
	
	using texture_container_t = std::map<std::string, Texture>;
	bool rgb_supported();

private:

	void set_first_elem(Element* elem) {
		m_first_elem = elem;
	}

	Element* m_first_elem = nullptr;

	const Instance* m_instance;
	VkDevice m_dev;

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
};

}

#endif
