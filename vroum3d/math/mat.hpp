#ifndef VROUM3D_MATH_MAT_HPP_INCLUDED
#define VROUM3D_MATH_MAT_HPP_INCLUDED

#include "vec.hpp"
#include <concepts>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <type_traits>
#include <cmath>

namespace Vroum3d::Math
{

struct mat_root {
  bool operator==(const mat_root& rhs) const = default;
  bool operator!=(const mat_root& rhs) const = default;
};

template<typename Mat>
constexpr bool is_mat = std::is_base_of_v<mat_root, Mat>;


template<typename FloatT, std::size_t N>
struct mat : mat_root
{
  using float_t = FloatT;
  constexpr static auto n = N;

  float_t data[n][n];

  template<typename Mat>
  class Acc
  {
    friend mat;
    Mat& m;
    std::size_t i;

    Acc(Mat& _m, std::size_t _i) : m(_m), i(_i) {}
  public:
    auto& operator[](std::size_t j) {return m.at(i, j);}
  };

  // Access with mathematical notation (row, col)
  Acc<mat> operator[](std::size_t idx) {return {*this, idx};}
  Acc<const mat> operator[](std::size_t idx) const {return {*this, idx};}


  float_t& at(std::size_t i, std::size_t j) {return data[j][i];}
  const float_t& at(std::size_t i, std::size_t j) const {return data[j][i];}

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, float_t&, std::size_t, std::size_t>, bool> = true>
  void foreach(const Fun& fun)
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j)
        fun(at(i, j), i, j);
  }


  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, float_t&>, bool> = true>
  void foreach(const Fun& fun)
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j)
        fun(at(i, j));
  }

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, float_t&, std::size_t, std::size_t>, bool> = true>
  void foreach(const Fun& fun) const
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j)
        fun(at(i, j), i, j);
  }


  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, float_t&>, bool> = true>
  void foreach(const Fun& fun) const
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j)
        fun(at(i, j));
  }

  mat() {}

  template<std::size_t Next>
  explicit mat(const mat<FloatT, Next>& from)
  {
    if constexpr (Next <= n)
    {
      operator=(identity());
      from.foreach([&](auto v, auto i, auto j){at(i, j) = v;});
    }
    else
      foreach([&](auto& v, auto i, auto j){v = from.at(i, j);});
  }

  static auto identity()
  {
    mat res;
    res.foreach([](auto& v, auto i, auto j){v = i == j ? float_t(1) : float_t(0);});
    return res;
  }

  static auto zero()
  {
    mat res;
    res.foreach([](auto& v){v = float_t(0);});
    return res;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> &&
            std::is_same_v<typename Vec::float_t, float_t>, bool> = true>
  static mat scale(const Vec& vec)
  {
    mat res = identity();

    std::size_t i(0);
    vec.foreach([&](auto val){res[i][i] = val; ++i;});
    return res;
  }


  template<typename Vec, std::enable_if_t<is_vec<Vec> &&
            std::is_same_v<typename Vec::float_t, float_t> &&
            n == Vec::n_comp() + 1, bool> = true>
  static mat translate(const Vec& vec)
  {
    mat res = identity();

    std::size_t i(0);
    vec.foreach([&](auto val){res[i][n - 1] = val; ++i;});
    return res;
  }

  mat operator*(const mat& rhs) const
  {
    mat res;
    res.foreach([&](auto& val, auto i, auto j)
      {
        val = 0;
        for(std::size_t k = 0; k != n; ++k)
          val += at(i, k) * rhs[k][j];
      }
    );

    return res;
  }

  mat& operator+=(const mat& rhs)
  {
    foreach([&](auto& val, auto i, auto j) {
      val += rhs[i][j];
    });
  }

  mat operator+(const mat& rhs) const
  {
    return mat(*this) += rhs;
  }

  mat& operator*=(const mat& rhs)
  {
    return operator=(*this * rhs);
  }

  mat& operator*=(auto scal)
  {
    foreach([&](auto& val) {
      val *= scal;
    });
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  mat operator*(auto scal) const
  {
    return mat(*this) *= scal;
  }
  

  template<typename Vec, std::enable_if_t<is_vec<Vec> &&
            std::is_same_v<typename Vec::float_t, float_t> &&
            Vec::n_comp() == n, bool> = true>
  Vec operator*(const Vec& vec) const
  {
    Vec res;
    std::size_t i(0);

    res.foreach([&](auto& out_val)
      {
        std::size_t j(0);
        vec.foreach([&](auto in_val)
                    {
                      out_val += in_val * at(i, j);
                      ++j;
                    });
        ++i;
      }
    );

    return res;
  }

  void disp(std::ostream& s) const
  {
    s << std::setfill(' ');
    s << "[";
    
    foreach([&](const auto& val, auto, auto j){
      if(!j) s << '\n';
      s << "\t" << std::setw(8) << val;
    });
    s << "\n]";
  }

  bool operator==(const mat& rhs) const = default;
  bool operator!=(const mat& rhs) const = default;
  

  template<typename Vec, std::enable_if_t<is_vec<Vec> &&
            std::is_same_v<typename Vec::float_t, float_t> &&
            Vec::n_comp() == n, bool> = true>
  friend Vec operator*(const Vec& vec, const mat& m)
  {
    Vec res;
    std::size_t i(0);

    res.foreach([&](auto& out_val)
      {
        std::size_t j(0);
        vec.foreach([&](auto in_val)
                    {
                      out_val += in_val * m.at(j, i);
                      ++j;
                    });
        ++i;
      }
    );

    return res;
  }
};

template<typename FloatT, std::size_t N>
std::ostream& operator<<(std::ostream& s, const mat<FloatT, N>& M)
{
  M.disp(s);
  return s;
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
auto scale(const Vec& v)
{
  return mat<typename Vec::float_t, Vec::n_comp()>::scale(v);
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
auto translate(const Vec& v)
{
  return mat<typename Vec::float_t, Vec::n_comp() + 1>::translate(v);
}

/** Projection matrix
  * X axis is screen x (towards right), Y axis screen y (towards up), Z axis is towards viewer.
  * @param angle viewing angle
  * @param ar aspect ratio (width / height)
  * @param nearz NearZ plane
  * @param farz FarZ plane
  */
template<typename FloatT = float>
auto proj(FloatT angle, FloatT ar, FloatT nearz, FloatT farz)
{
  typedef mat<FloatT, 4> res_t;

  // Nearz, farz associated parameters for transform z := a + b / z

  FloatT a = 1 / (1 - nearz / farz);
  FloatT b = a * nearz;

  FloatT angle_tan = std::tan(angle / 2);

  res_t res = res_t::scale(generic_vec4<FloatT>(1/angle_tan, -ar / angle_tan, -a, 0));
  res[2][3] = -b;
  res[3][2] = -1;

  return res;
}

template<typename FloatT, std::size_t n>
mat<FloatT, n> transpose(const mat<FloatT, n>& from)
{
  mat<FloatT, n> res;
  res.foreach([&](auto& v, auto i, auto j){v = from.at(j, i);});
  return res;
}

template<std::size_t axis, typename FloatT = float>
auto rotate(FloatT angle)
{
  typedef mat<FloatT, 3> res_t;
  static_assert(axis < 3);

  res_t res = res_t::identity();
  
  constexpr std::size_t idx_a = axis == 0 ? 1 : 0;
  constexpr std::size_t idx_b = axis == 2 ? 1 : 2;

  auto cos = std::cos(angle);
  auto sin = std::sin(angle);

  res[idx_a][idx_a] = res[idx_b][idx_b] = cos;
  res[idx_b][idx_a] = -(res[idx_a][idx_b] = sin);

  return res;
}

template<typename FloatT>
auto ortho_transform(const generic_vec3<FloatT>& projx, const generic_vec3<FloatT>& projy)
{
  return ortho_transform(projx, projy, cross(projx, projy));
}

template<typename FloatT>
auto ortho_transform(const generic_vec3<FloatT>& projx, const generic_vec3<FloatT>& projy, const generic_vec3<FloatT>& projz)
{
  mat<FloatT, 3> res;

  int idx(0);
  projx.foreach([&](auto val){res[0][idx++] = val;});
  idx = 0;
  projy.foreach([&](auto val){res[1][idx++] = val;});
  idx = 0;
  projz.foreach([&](auto val){res[2][idx++] = val;});

  return res;
}

typedef mat<float, 4> mat4;
typedef mat<float, 3> mat3;
typedef mat<float, 2> mat2;

}

#endif 
