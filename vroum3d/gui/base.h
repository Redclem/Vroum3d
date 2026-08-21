#ifndef VROUM3D_GUI_BASE_H_INCLUDED
#define VROUM3D_GUI_BASE_H_INCLUDED

#include "common.h"
#include "element.h"

#include "../core/pipeline.h"
#include "../core/instance.h"

#include <vulkan/vulkan_core.h>

#include <map>

namespace Vroum3d::Gui
{

class Element;

using namespace Core;

class Base : public AssignDestroy<Base>
{
public:

private:
	struct Texture
	{
		VkHandle<VkImage> img;
		VkHandle<VkImageView> view;
		Allocator::OwnedMemory mem;

		std::uint32_t w, h;
	};
	
	using texture_container_t = std::map<std::string, Texture>;
	bool rgb_supported();


	Element *m_root_elem = nullptr, *m_first_element;

	DisplayInstance* m_instance;
	PipelineResource * m_pipe_res;
	VkDevice m_device;

	VkHandle<VkBuffer> m_buffer;
	Allocator::OwnedMemory m_buffer_mem;
	texture_container_t m_textures;
	bool m_rgb;
	
	Pipeline m_fill_pipe;
	RenderCommands m_render_commands;
	VkHandle<VkCommandPool> m_cmd_pool;
	std::vector<VkCommandBuffer> m_cmd_bufs;

	void init_command_buffers();

	void build_render_buffer(uint32_t image_idx);

public:

	~Base() {
		destroy();
	}

	void set_root_elem(Element* elem) {
		m_root_elem = elem;
	}

	void destroy();
	auto instance() const {return m_instance;}

	Base(DisplayInstance& inst, PipelineResource& pr);

	VkDevice device() const {return m_device;}

	void init();

	void arrange()
	{
		if(!m_root_elem) return;
		m_root_elem->set_position({0, 0, m_instance->w(), m_instance->h()});
		m_root_elem->arrange();
	}

	void register_element(Element* elem)
	{
		elem->m_next_element = m_first_element;
		m_first_element = elem;
	}

	void render();

private:
	void assign_buffer_space();
	void allocate_buffer();
};

}

#endif
