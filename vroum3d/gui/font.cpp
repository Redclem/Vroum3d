#include "font.h"
#include <array>
#include <fstream>
#include <memory>
#include <algorithm>

using namespace Vroum3d::Gui;

void Font::open(const char* pth)
{
  std::ifstream file(pth, std::ios::binary | std::ios::ate);
  std::size_t size = file.tellg();

  m_buffer = std::make_unique<char[]>(size);
  file.seekg(0, std::ios::beg);

  file.read(m_buffer.get(), size);
  stbtt_InitFont(&m_info, reinterpret_cast<unsigned char*>(m_buffer.get()), 0);
}

std::pair<Bitmap<Font::font_px_t>, Font::atlas_glyphs_t> Font::render_char_atlas()
{
  Bitmap<Font::font_px_t> res(1024, 1024);
  stbtt_pack_context ctx;

  check(stbtt_PackBegin(&ctx, res.data(), res.w(), res.h(), res.w(), 1, nullptr));
  stbtt_PackSetSkipMissingCodepoints(&ctx, 1);

  std::vector<stbtt_packedchar> packed_chars(0x80 - 0x21 + 0x100 - 0xc0);

  std::array<stbtt_pack_range, 2> ranges = {{
    {
      12,
      0x21,
      nullptr,
      0x80 - 0x21,
      packed_chars.data(),
      0, 0
    },
    {
      12,
      0xc0,
      nullptr,
      0x100 - 0xc0,
      packed_chars.data() + 0x80 - 0x21,
      0, 0
    }
  }};

  stbtt_PackFontRanges(&ctx, reinterpret_cast<unsigned char*>(m_buffer.get()), 0, ranges.data(), ranges.size());

  uint32_t req_w(0), req_h(0);
  for(auto& elem : packed_chars)
  {
    req_w = std::max<uint32_t>(req_w, elem.x1);
    req_h = std::max<uint32_t>(req_h, elem.y1);
  }

  stbtt_PackEnd(&ctx);

  atlas_glyphs_t glyphs;

  float wf(req_w), hf(req_h);

  for(auto& elem : ranges)
  {
    uint32_t code(elem.first_unicode_codepoint_in_range);
    for(auto iter = elem.chardata_for_range, end = elem.chardata_for_range + elem.num_chars; iter != end; ++iter, ++code)
    {
      glyphs.emplace(code, Glyph{
        {float(iter->x0) / wf, float(iter->y0) / hf},
        {float(iter->x1) / wf, float(iter->y1) / hf},
        {iter->xoff, iter->yoff},
        iter->xadvance
      });
    }
  }

  return {res.sub(0, 0, req_w, req_h), glyphs};
}
