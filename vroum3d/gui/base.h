#ifndef VROUM3D_GUI_BASE_H_INCLUDED
#define VROUM3D_GUI_BASE_H_INCLUDED

#include "common.h"

#include "../core/pipeline.h"
#include "../core/instance.h"
#include "../core/objects.h"

#include "../../ext/stb_image.h"
#include "font.h"
#include "bitmap.hpp"

#include <string_view>
#include <vulkan/vulkan_core.h>

#include <map>

namespace Vroum3d::Gui
{

class Base;

class Element
{
	friend class Base;

protected:
	Base* m_base;
	Rect m_position;
	VkDeviceSize m_buffer_offset;
	Element* m_next_element;
	
public:

	auto next_element() const {return m_next_element;}

	void set_buffer_offset(VkDeviceSize offset) {m_buffer_offset = offset;}

	constexpr Element(Base* base) : m_base(base) {
	}

	Base* base() const {return m_base;}

	constexpr static VkDeviceSize c_buffer_size = 0;

	/** Get required buffer size for this element
	 * Does not include child elements!
	 * Should be constant or at least fixed after init */
	virtual VkDeviceSize buffer_size() const {return c_buffer_size;};

	/** Init Element : 
	 * - Call ancestor's init function (including if deriving directly from Element !)
	 * - Init child elements
	 * - Require needed textures from base using require_texture
	 * - Build required vk objects
	 */
	virtual void init();

	/** Record upload commands for data upload on initialization / size change
	 * Do not call on child elements
	 * \param buffer_data_ptr Pointer to area of memory mapped to buffer. Does not account of offset of current element.
	 */
	virtual void upload_buffer(char * buffer_data_ptr) = 0;

  /** Get minimal element dimension.
   * This function is usually called before arrange to gather information about sizes of child elements.
   */
  virtual Extent get_min_dimensions() {return {0, 0};}

	/** Update inner state on position change.
	 * Should arrange child elements / elements contained */
	virtual void arrange();

	const Rect& position() const {return m_position;}
	void set_position(const Rect& p) {m_position = p;}

	/** Record render commands in given struct
	 * Also record appropriate child commands */
	virtual void record_render_commands(RenderCommands& rc) = 0;

  /**
   * Utility functions and structs for rendering
   * Frame structure
   */

  // Data required to render a frame
	struct FrameRenderData
	{
		std::array<Point, 10> points;
	};

  /**
   * Write data for frame rendering
   *
   * @param rd render data
   * @param out_pos Rectangle describing the outside of the frame
   * @param width Frame width
   *
   */
  static void frame_write_buffer(char* buffer_data_ptr, Rect outer_pos, px_t width)
  {
    FrameRenderData& rd = *reinterpret_cast<FrameRenderData*>(buffer_data_ptr);

    auto pts_outer = outer_pos.rect_points();
    outer_pos.shrink(width);
    auto pts_inner = outer_pos.rect_points();

    rd.points[0] = pts_outer[0];
    rd.points[1] = pts_inner[0];
    rd.points[2] = pts_outer[1];
    rd.points[3] = pts_inner[1];
    rd.points[4] = pts_outer[2];
    rd.points[5] = pts_inner[2];
    rd.points[6] = pts_outer[3];
    rd.points[7] = pts_inner[3];
    rd.points[8] = pts_outer[0];
    rd.points[9] = pts_inner[0];
  }

  static void frame_render(RenderCommands& rc, VkDeviceSize offset)
  {
    rc.fill(offset, 10);
  }
  /**
   * Text utility functions
   */

  struct TexturedPoint
  {
    Point pos;
    Math::vec2 uv;
  };

  struct GlyphData
  {
    TexturedPoint pts[4];
  };

  /**
   * Compute buffer size to reserve to render text
   *
   * @param glyphs Glyphs to use to check if code point is rendered
   * @param str The UTF-8 string to be rendered
   *
   */
  static VkDeviceSize text_byte_size(const Font::GlyphAtlas& glyphs, std::string_view str) 
  {
    return glyphs.glyph_count(str) * (sizeof(Element::GlyphData) + 5 * sizeof(std::uint16_t)) - sizeof(std::uint16_t);
  }

  /**
   * Export text buffer data
   *
   * @param glyphs Glyphs to use
   * @param str String to be rendered
   * @param buffer_data_ptr Pointer to buffer space receiving data. Minimal size can be computed using text_byte_size
   * @param text_origin Origin of the text (position of first glyph)
   * @return The number of glyphs to render
   *
   */
  static std::uint32_t text_write_buffer(char* buffer_data_ptr, const Font::GlyphAtlas& glyphs, std::string_view str, Math::vec2 text_origin) 
  {
    auto* glyph_out_iter(reinterpret_cast<GlyphData*>(buffer_data_ptr));

    std::uint16_t idx(0);

    float adv(0.0f);

    std::uint32_t n_glyphs = 0;

    auto proc_vert = [&](char32_t point)
      {
        auto iter = glyphs.find(point);
        if(iter == glyphs.end()) return;

        const auto& g = iter->second;

        glyph_out_iter->pts[0] = {{adv + text_origin.x + g.offset.x, g.offset.y + text_origin.y}, g.start};
        glyph_out_iter->pts[1] = {{adv + text_origin.x + g.offset.x + g.w, g.offset.y + text_origin.y}, {g.end.x, g.start.y}};
        glyph_out_iter->pts[2] = {{adv + text_origin.x + g.offset.x, g.offset.y + g.h + text_origin.y}, {g.start.x, g.end.y}};
        glyph_out_iter->pts[3] = {{adv + text_origin.x + g.offset.x + g.w, g.offset.y + g.h + text_origin.y}, g.end};

        glyph_out_iter++;
        adv += g.adv;
        n_glyphs++;
      };

    glyphs.iterate_unicode_points(str, proc_vert);

    std::uint16_t * index_iter = reinterpret_cast<std::uint16_t*>(glyph_out_iter);
    
    for(std::size_t g(0); g != n_glyphs; ++g)
    {
      if(g)
        *(index_iter++) = 0xFFFF;

      *(index_iter++) = idx++;
      *(index_iter++) = idx++;
      *(index_iter++) = idx++;
      *(index_iter++) = idx++;
    }

    return n_glyphs;
  }

  /**
   * Record label render commands
   *
   * @param rc RenderCommands to record into
   * @param buffer offset for label data
   * @param n_glyphs number of glyphs to render
   * @param set_index index of the glyph atlas into the descriptor set
   * @return Type and description of the returned value.
   */
  static void text_render(RenderCommands& rc, VkDeviceSize offset, std::size_t n_glyphs, std::uint32_t set_index)
  {
    if(n_glyphs)
      rc.text(offset, offset + n_glyphs * sizeof(GlyphData),
              n_glyphs * 5 - 1, set_index);
  }
};

using namespace Core;

class Base : public AssignDestroy<Base>
{
public:
	struct Texture
	{
		VkHandle<VkImage> img;
		VkHandle<VkImageView> view;
		Allocator::OwnedMemory mem;

		std::uint32_t w, h;
		std::uint32_t set_index;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
  };

  struct FontAtlas
  {
    VkHandle<VkImage> img;
    VkHandle<VkImageView> view;
    Allocator::OwnedMemory mem;
  
    Font::atlas_glyphs_t glyphs;
    std::uint32_t set_index;
  };

	using texture_ptr_t = Texture*;
  using font_ptr_t = FontAtlas*;

  constexpr static VkDeviceSize c_element_buffer_alignment = 4;

  static_assert((c_element_buffer_alignment & (c_element_buffer_alignment - 1)) == 0);

private:
	
	using texture_container_t = std::map<std::string, Texture>;
  using font_container_t = std::map<std::string, FontAtlas>;

	Element *m_root_elem = nullptr, *m_first_element = nullptr;

	DisplayInstance* m_instance;
	PipelineResource * m_pipe_res;
	VkDevice m_device;

	VkHandle<VkBuffer> m_buffer;
	Allocator::OwnedMemory m_buffer_mem;
	VkDeviceSize m_buffer_size = 0;
	char * m_mapped_buffer_ptr = nullptr;
	texture_container_t m_textures;
  font_container_t m_fonts;
	
	Pipeline m_fill_pipe, m_textured_pipe, m_text_pipe;
	RenderCommands m_render_commands;
	VkHandle<VkCommandPool> m_cmd_pool;
	std::vector<VkCommandBuffer> m_cmd_bufs;
	VkHandle<VkDescriptorPool> m_desc_pool;
	VkDescriptorSet m_tex_des_set;

  font_ptr_t m_default_font = nullptr;

public:

	~Base() {
		destroy();
	}

	void set_root_elem(Element* elem) {
		m_root_elem = elem;
	}

	void destroy();
	auto instance() const {return m_instance;}

  font_ptr_t default_font() const {return m_default_font;}

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

	/** Require a texture to be loaded for future use. Should be called before or during base / element initialization
	 * \param pth path of the texture to load
	 * \warning Does not load the texture immediately. Texture is loaded after all required textures have been gathered and all elements initialized.
	 * */

	template<typename T>
	texture_ptr_t require_texture(T&& pth)
	{
		return &m_textures.emplace(std::forward<T>(pth), Texture{}).first->second;
	}

  template<typename Pth>
  font_ptr_t require_font(Pth&& pth)
  {
    return &m_fonts.emplace(std::forward<Pth>(pth), FontAtlas{}).first->second;
  }

  template<typename Pth>
  void set_default_font(Pth&& pth)
  {
    m_default_font = require_font(std::forward<Pth>(pth));
  }

	void render();

private:

	struct StbiDeleter {void operator()(unsigned char* ptr) {stbi_image_free(ptr);}};
	using StbiPtr = std::unique_ptr<unsigned char, StbiDeleter>;

	void init_command_buffers();
	void build_render_buffer();
	void create_descriptor_set();

  VkDeviceSize load_textures(std::vector<StbiPtr>& textures);
  void write_texture_upload_commands(CommandBuffer& cmd_buffer, const std::vector<StbiPtr>& textures, char* dt, VkBuffer upl_buffer, VkDeviceSize ofs);

  using font_bitmap_t = Bitmap<uint8_t>;

  VkDeviceSize load_font_bitmaps(std::vector<font_bitmap_t>& bitmaps);
  void write_font_upload_commands(CommandBuffer& cmd_buffer, const std::vector<font_bitmap_t>& bitmaps, char* dt, VkBuffer upl_buffer, VkDeviceSize ofs);
};

}

#endif
