#ifndef VROUM3D_GUI_FONT_H_INCLUDED
#define VROUM3D_GUI_FONT_H_INCLUDED

#include "../../ext/stb_truetype.h"
#include "../utility.h"
#include "../gui/bitmap.hpp"
#include "../math/vec.hpp"

#include <cstdint>
#include <map>
#include <memory>
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

  using atlas_glyphs_t = std::map<uint32_t, Glyph>;
  
  std::pair<Bitmap<font_px_t>, atlas_glyphs_t> render_char_atlas();

private:
  std::unique_ptr<char[]> m_buffer;
  stbtt_fontinfo m_info;
};

}

#endif // !VROUM3D_GUI_FONT_H_INCLUDED
