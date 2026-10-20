#ifndef VROUM3D_MATH_MAT_HPP_INCLUDED
#define VROUM3D_MATH_MAT_HPP_INCLUDED

#include "base.hpp"
#include "vec.hpp"

#include <cmath>
#include <concepts>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <type_traits>

namespace Vroum3d::Math
{

/** Row major */
template<typename ScalT, std::size_t N>
struct mat : mat_root
{
  using scal_t = ScalT;
  static constexpr auto n = N;

  scal_t data[n][n];

  template<typename Mat>
  class Acc {
    friend mat;
    Mat &m;
    std::size_t i;

    Acc(Mat &_m, std::size_t _i) : m(_m), i(_i) {}

  public:
    auto &operator[](std::size_t j) { return m.at(i, j); }
  };

  // Access with mathematical notation (row, col)
  Acc<mat> operator[](std::size_t idx) { return {*this, idx}; }

  Acc<const mat> operator[](std::size_t idx) const { return {*this, idx}; }

  scal_t &at(std::size_t i, std::size_t j) { return data[j][i]; }

  const scal_t &at(std::size_t i, std::size_t j) const { return data[j][i]; }

  template<typename FScalT>
  constexpr mat(const mat<FScalT, N> &from)
  {
    from.foreach([&](auto &val, auto i, auto j) { at(i, j) = val; });
  }

  template<typename FScalT>
  constexpr mat &operator=(const mat<FScalT, N> &from)
  {
    from.foreach([&](auto &val, auto i, auto j) { at(i, j) = val; });
    return *this;
  }

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, scal_t &, std::size_t, std::size_t>, bool> = true>
  void foreach(const Fun &fun)
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j) fun(at(i, j), i, j);
  }

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, scal_t &>, bool> = true>
  void foreach(const Fun &fun)
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j) fun(at(i, j));
  }

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, scal_t &, std::size_t, std::size_t>, bool> = true>
  void foreach(const Fun &fun) const
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j) fun(at(i, j), i, j);
  }

  template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, scal_t &>, bool> = true>
  void foreach(const Fun &fun) const
  {
    for(std::size_t i(0); i != n; ++i)
      for(std::size_t j(0); j != n; ++j) fun(at(i, j));
  }

  mat() {}

  template<std::size_t Next>
  explicit mat(const mat<ScalT, Next> &from)
  {
    if constexpr(Next <= n)
    {
      operator=(identity());
      from.foreach([&](auto v, auto i, auto j) { at(i, j) = v; });
    }
    else
      foreach([&](auto &v, auto i, auto j) { v = from.at(i, j); });
  }

  static auto identity()
  {
    mat res;
    res.foreach([](auto &v, auto i, auto j) { v = i == j ? scal_t(1) : scal_t(0); });
    return res;
  }

  static auto zero()
  {
    mat res;
    res.foreach([](auto &v) { v = scal_t(0); });
    return res;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && std::is_same_v<typename Vec::scal_t, scal_t>, bool> = true>
  static mat scale(const Vec &vec)
  {
    mat res = identity();

    std::size_t i(0);
    vec.foreach([&](auto val) {
      res[i][i] = val;
      ++i;
    });
    return res;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && std::is_same_v<typename Vec::scal_t, scal_t> && n == Vec::n_comp() + 1, bool> = true>
  static mat translate(const Vec &vec)
  {
    mat res = identity();

    std::size_t i(0);
    vec.foreach([&](auto val) {
      res[i][n - 1] = val;
      ++i;
    });
    return res;
  }

  mat operator*(const mat &rhs) const
  {
    mat res;
    res.foreach([&](auto &val, auto i, auto j) {
      val = 0;
      for(std::size_t k = 0; k != n; ++k) val += at(i, k) * rhs[k][j];
    });

    return res;
  }

  mat &operator+=(const mat &rhs)
  {
    foreach([&](auto &val, auto i, auto j) { val += rhs[i][j]; });
  }

  mat operator+(const mat &rhs) const { return mat(*this) += rhs; }

  mat &operator*=(const mat &rhs) { return operator=(*this* rhs); }

  mat &operator*=(auto scal)
  {
    foreach([&](auto &val) { val *= scal; });
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  mat operator*(auto scal) const
  {
    return mat(*this) *= scal;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && std::is_same_v<typename Vec::scal_t, scal_t> && Vec::n_comp() == n, bool> = true>
  Vec operator*(const Vec &vec) const
  {
    Vec res;
    std::size_t i(0);

    res.foreach([&](auto &out_val) {
      std::size_t j(0);
      vec.foreach([&](auto in_val) {
        out_val += in_val * at(i, j);
        ++j;
      });
      ++i;
    });

    return res;
  }

  void disp(std::ostream &s) const
  {
    s << std::setfill(' ');
    s << "[";

    foreach([&](const auto &val, auto, auto j) {
      if(!j)
        s << '\n';
      s << "\t" << std::setw(8) << val;
    });
    s << "\n]";
  }

  bool operator==(const mat &rhs) const = default;
  bool operator!=(const mat &rhs) const = default;

  template<typename Vec, std::enable_if_t<is_vec<Vec> && std::is_same_v<typename Vec::scal_t, scal_t> && Vec::n_comp() == n, bool> = true>
  friend Vec operator*(const Vec &vec, const mat &m)
  {
    Vec res;
    std::size_t i(0);

    res.foreach([&](auto &out_val) {
      std::size_t j(0);
      vec.foreach([&](auto in_val) {
        out_val += in_val * m.at(j, i);
        ++j;
      });
      ++i;
    });

    return res;
  }
};

template<typename ScalT, std::size_t N>
std::ostream &operator<<(std::ostream &s, const mat<ScalT, N> &M)
{
  M.disp(s);
  return s;
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
auto scale(const Vec &v)
{
  return mat<typename Vec::scal_t, Vec::n_comp()>::scale(v);
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
auto translate(const Vec &v)
{
  return mat<typename Vec::scal_t, Vec::n_comp() + 1>::translate(v);
}

/** Projection matrix
 * X axis is screen x (towards right), Y axis screen y (towards up), Z axis is towards viewer.
 * @param angle viewing angle
 * @param ar aspect ratio (width / height)
 * @param nearz NearZ plane
 * @param farz FarZ plane
 */
template<typename ScalT = float>
auto proj(ScalT angle, ScalT ar, ScalT nearz, ScalT farz)
{
  typedef mat<ScalT, 4> res_t;

  // Nearz, farz associated parameters for transform z := a + b / z

  ScalT a = 1 / (1 - nearz / farz);
  ScalT b = a * nearz;

  ScalT angle_tan = std::tan(angle / 2);

  res_t res = res_t::scale(generic_vec4<ScalT>(1 / angle_tan, ar / angle_tan, -a, 0));
  res[2][3] = -b;
  res[3][2] = -1;

  return res;
}

template<typename ScalT, std::size_t n>
mat<ScalT, n> transpose(const mat<ScalT, n> &from)
{
  mat<ScalT, n> res;
  res.foreach([&](auto &v, auto i, auto j) { v = from.at(j, i); });
  return res;
}

template<std::size_t axis, typename ScalT = float>
auto rotate(ScalT angle)
{
  typedef mat<ScalT, 3> res_t;
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

template<typename ScalT>
auto ortho_transform(const generic_vec3<ScalT> &projx, const generic_vec3<ScalT> &projy)
{
  return ortho_transform(projx, projy, cross(projx, projy));
}

template<typename ScalT>
auto ortho_transform(const generic_vec3<ScalT> &projx, const generic_vec3<ScalT> &projy, const generic_vec3<ScalT> &projz)
{
  mat<ScalT, 3> res;

  int idx(0);
  projx.foreach([&](auto val) { res[0][idx++] = val; });
  idx = 0;
  projy.foreach([&](auto val) { res[1][idx++] = val; });
  idx = 0;
  projz.foreach([&](auto val) { res[2][idx++] = val; });

  return res;
}

template<typename ScalT>
mat<ScalT, 3> quater_rotate(ScalT x, ScalT y, ScalT z, ScalT s)
{
  auto x2 = x * x;
  auto y2 = y * y;
  auto z2 = z * z;

  mat<ScalT, 3> res;
  res[0][0] = 1 - 2 * (y2 + z2);
  res[0][1] = 2 * (x * y - s * z);
  res[0][2] = 2 * (x * z + s * y);
  res[1][0] = 2 * (x * y + s * z);
  res[1][1] = 1 - 2 * (x2 + z2);
  res[1][2] = 2 * (y * z - s * x);
  res[2][0] = 2 * (x * z - s * y);
  res[2][1] = 2 * (y * z + s * x);
  res[2][2] = 1 - 2 * (x2 + y2);

  return res;
}

/** Applicable for ROTATE * SCALE matrices (ROTATE can be any isometric) */
template<typename ScalT>
mat<ScalT, 3> associated_normal_transform(const mat<ScalT, 3> &t)
{
  using vec_t = generic_vec3<ScalT>;

  vec_t norms(vec_t(t[0][0], t[1][0], t[2][0]).norm(), vec_t(t[0][1], t[1][1], t[2][1]).norm(), vec_t(t[0][2], t[1][2], t[2][2]).norm());

  norms.x = 1 / norms.x / norms.x;
  norms.y = 1 / norms.y / norms.y;
  norms.z = 1 / norms.z / norms.z;

  return t * scale(norms);
}

typedef mat<float, 4> mat4;
typedef mat<float, 3> mat3;
typedef mat<float, 2> mat2;

} // namespace Vroum3d::Math


#endif
