#ifndef VROUM3D_GUI_BITMAP_HPP_INCLUDED
#define VROUM3D_GUI_BITMAP_HPP_INCLUDED

#include <cstdint>
#include <memory>
#include <sys/types.h>
namespace Vroum3d::Gui
{

/** Simple bitmap class with custom pixel type, without padding */
template<typename Px>
class Bitmap
{
public:
  using px_t = Px;
  using uint32_t = std::uint32_t;

  void clear()
  {
    m_w = m_h = 0;
    m_data = nullptr;
  }

  Bitmap() {}
  
  void create(uint32_t w, uint32_t h)
  {
    m_w = w;
    m_h = h;
    m_data = std::make_unique<px_t[]>(w * h);
  }

  Bitmap(uint32_t w, uint32_t h) 
  {
    create(w, h);
  }

  auto w() const {return m_w;}
  auto h() const {return m_h;}
  auto data() const {return m_data.get();}

  auto& at(auto x, auto y) const {return m_data[x + m_w * y];}
  auto& at(auto x, auto y) {return m_data[x + m_w * y];}

  Bitmap sub(uint32_t x, uint32_t y, uint32_t w, uint32_t h) const
  {
    Bitmap res(w, h);

    for(uint32_t iterx(0); iterx != w; ++iterx)
      for(uint32_t itery(0); itery != h; ++itery)
        res.at(iterx, itery) = at(x + iterx, y + itery);

    return res;
  }


private:
  uint32_t m_w = 0, m_h = 0;
  std::unique_ptr<px_t[]> m_data;
};

}

#endif // !VROUM3D_GUI_BITMAP_HPP_INCLUDED
