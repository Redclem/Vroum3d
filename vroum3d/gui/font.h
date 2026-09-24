#ifndef VROUM3D_GUI_FONT_H_INCLUDED
#define VROUM3D_GUI_FONT_H_INCLUDED

#include "../../ext/stb_truetype.h"
#include "../utility.h"
#include "bitmap.hpp"
#include "common.h"
#include "../math/vec.hpp"

#include <cstdint>
#include <locale>
#include <map>
#include <memory>
#include <string_view>
#include <utility>


namespace Vroum3d::Gui
{

class Font : public NonCopyable
{
public:
  using font_px_t = unsigned char;

  Font(const char* pth) {open(pth);}
  Font() {}

  void open(const char* pth);

  struct Glyph
  {
    Math::vec2 start, end;
    Math::vec2 offset;
    float adv;
    uint16_t w, h;
  };

  struct GlyphAtlas : std::map<uint32_t, Glyph>
  {
    Rect compute_text_size(std::string_view str) const;
    std::size_t glyph_count(std::string_view str) const;

    template<typename Fun>
    static void iterate_unicode_points(std::string_view str, Fun&& f);
  };

  using atlas_glyphs_t = GlyphAtlas;
  
  std::pair<Bitmap<font_px_t>, atlas_glyphs_t> render_char_atlas();

private:
  std::unique_ptr<char[]> m_buffer;
  stbtt_fontinfo m_info;
};

template<typename Fun>
void Font::GlyphAtlas::iterate_unicode_points(std::string_view str, Fun&& f)
{
  using cvrt_t = std::codecvt<char32_t, char, std::mbstate_t>;
  struct Converter : cvrt_t
  {
    using cvrt_t::cvrt_t;
    ~Converter() {}
  };

  Converter cvrt;
  std::mbstate_t state;

  char32_t out;

  auto iter = str.data();
  auto end_iter = iter + str.length();

  std::codecvt_base::result code;

  do {
    char32_t *ptr;
    code = cvrt.in(state, iter, end_iter, iter, &out, &out+1, ptr);

    if(code == std::codecvt_base::result::error) return;
    if(code == std::codecvt_base::result::noconv) return;

    f(out);

  } while (code != std::codecvt_base::result::ok);
}

}

#endif // !VROUM3D_GUI_FONT_H_INCLUDED
