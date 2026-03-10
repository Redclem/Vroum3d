#ifndef VROUM3D_MATH_VEC_HPP_INCLUDED
#define VROUM3D_MATH_VEC_HPP_INCLUDED

#include <algorithm>
#include <ostream>
#include <cmath>
#include <type_traits>

namespace Vroum3d::Math
{

struct vec_root {};

template<typename T>
constexpr bool is_vec = std::is_base_of_v<vec_root, T>;

template<typename Deriv>
struct vec_base : vec_root
{
  using deriv_t = Deriv;

  constexpr static std::size_t n_comp()
  {
    std::size_t nc(0);
    deriv_t().foreach([&](auto){nc++;});
    return nc;
  }

  constexpr auto& drv() {return *static_cast<deriv_t*>(this);}
  constexpr const auto& drv() const {return *static_cast<const deriv_t*>(this);}

  constexpr deriv_t& operator+=(const vec_base& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, const auto& b){a += b;});
    return drv();
  }

  constexpr deriv_t& operator-=(const vec_base& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, const auto& b){a -= b;});
    return drv();
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  constexpr deriv_t& operator+=(Scal rhs)
  {
    drv().foreach([&](auto& a){a += rhs;});
    return drv();
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  constexpr deriv_t& operator-=(Scal rhs)
  {
    drv().foreach([&](auto& a){a -= rhs;});
    return drv();
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  constexpr deriv_t& operator*=(Scal scal)
  {
    drv().foreach([&](auto& a){a *= scal;});
    return drv();
  }

  template<typename Scal, std::enable_if_t<!is_vec<Scal>, bool> = true>
  constexpr deriv_t& operator/=(Scal scal)
  {
    drv().foreach([&](auto& a){a /= scal;});
    return drv();
  }

  constexpr deriv_t operator+(const auto& rhs) const
  {
    return deriv_t(drv()) += rhs;
  }

  constexpr deriv_t operator-(const auto& rhs) const
  {
    return deriv_t(drv()) -= rhs;
  }

  constexpr deriv_t operator*(const auto& rhs) const
  {
    return deriv_t(drv()) *= rhs;
  }

  constexpr deriv_t operator/(const auto& rhs) const
  {
    return deriv_t(drv()) /= rhs;
  }
  constexpr vec_base() {}

  constexpr void disp(std::ostream& s) const 
  {
    s << '(';

    bool first(true);
    drv().foreach([&](const auto& elem)
                  {
                    if(!first)
                    {
                      s << ", ";
                    }
                    else {
                      first = false;
                    }
                    s << elem;
                  });
    s << ')';
  }

  constexpr bool operator==(const vec_base& rhs) const
  {
    bool eq = true;
    drv().foreach_paired(rhs.drv(), [&](const auto& a, const auto& b) {eq = eq && a == b;});
    return eq;
  }

  constexpr bool operator!=(const vec_base& rhs) const
  {
    bool diff = false;
    drv().foreach_paired(rhs.drv(), [&](const auto& a, const auto& b) {diff = diff || a != b;});
    return diff;
  }

  constexpr auto dot(const vec_base& rhs) const
  {
    typename deriv_t::float_t res(0);
    drv().foreach_paired(rhs.drv(), [&](const auto& a, const auto& b) {res += a * b;});

    return res;
  }

  constexpr auto norm2() const
  {
    typename deriv_t::float_t res(0);
    drv().foreach([&](const auto& a) {res += a * a;});

    return res;
  }

  constexpr auto norm() const
  {
    return std::sqrt(norm2());
  }

  constexpr auto to_array() const
  {
    std::array<typename deriv_t::float_t, n_comp()> res;
    auto iter = res.begin();

    drv().foreach([&](auto val){*(iter++) = val;});
    return res;
  }

protected:
private:
};


template<typename T>
std::ostream& operator<<(std::ostream& s, const vec_base<T>& vb)
{
  vb.disp(s);
  return s;
}

template<typename Scal, typename T, std::enable_if_t<!is_vec<Scal>, bool> = true>
constexpr auto operator*(Scal lhs, const vec_base<T>& rhs)
{
  return rhs * lhs;
}

template<typename Scal, typename T, std::enable_if_t<!is_vec<Scal>, bool> = true>
constexpr auto operator+(Scal lhs, const vec_base<T>& rhs)
{
  return rhs + lhs;
}

template<typename FloatT>
struct base_generic_vec2
{
  using float_t = FloatT;

  float_t x, y;

  constexpr void foreach(const auto& fun)
  {
    fun(x);
    fun(y);
  }

  constexpr void foreach_paired(const base_generic_vec2& rhs, const auto& fun)
  {
    fun(x, rhs.x);
    fun(y, rhs.y);
  }

  constexpr void foreach(const auto& fun) const
  {
    fun(x);
    fun(y);
  }

  constexpr void foreach_paired(const base_generic_vec2& rhs, const auto& fun) const
  {
    fun(x, rhs.x);
    fun(y, rhs.y);
  }

  constexpr base_generic_vec2(float_t _x, float_t _y) : x(_x), y(_y) {}
  constexpr base_generic_vec2(float_t scal = 0) : x(scal), y(scal) {}
};

template<typename FloatT>
struct base_generic_vec3 : base_generic_vec2<FloatT>
{
  using base_t = base_generic_vec2<FloatT>;
  using typename base_t::float_t;
    
  float z;

  constexpr void foreach(const auto& fun)
  {
    base_t::foreach(fun);
    fun(z);
  }

  constexpr void foreach_paired(const base_generic_vec3& rhs, const auto& fun)
  {
    base_t::foreach_paired(rhs, fun);
    fun(z, rhs.z);
  }

  constexpr void foreach(const auto& fun) const
  {
    base_t::foreach(fun);
    fun(z);
  }

  constexpr void foreach_paired(const base_generic_vec3& rhs, const auto& fun) const
  {
    base_t::foreach_paired(rhs, fun);
    fun(z, rhs.z);
  }

  constexpr base_generic_vec3(float_t _x, float_t _y, float_t _z) : base_t(_x, _y), z(_z) {}
  constexpr base_generic_vec3(float_t scal = 0) : base_t(scal), z(scal) {}

};

template<typename FloatT>
struct base_generic_vec4 : base_generic_vec3<FloatT>
{
  using base_t = base_generic_vec3<FloatT>;
  using typename base_t::float_t;
    
  float w;

  constexpr void foreach(const auto& fun)
  {
    base_t::foreach(fun);
    fun(w);
  }

  constexpr void foreach_paired(const base_generic_vec4& rhs, const auto& fun)
  {
    base_t::foreach_paired(rhs, fun);
    fun(w, rhs.w);
  }

  constexpr void foreach(const auto& fun) const
  {
    base_t::foreach(fun);
    fun(w);
  }

  constexpr void foreach_paired(const base_generic_vec4& rhs, const auto& fun) const
  {
    base_t::foreach_paired(rhs, fun);
    fun(w, rhs.w);
  }

  constexpr base_generic_vec4(float_t _x, float_t _y, float_t _z, float_t _w) : base_t(_x, _y, _z), w(_w) {}
  constexpr base_generic_vec4(float_t scal = 0) : base_t(scal), w(scal) {}

};

template<typename FloatT>
struct generic_vec2 : base_generic_vec2<FloatT>, vec_base<generic_vec2<FloatT>>
{
  using base_t = base_generic_vec2<FloatT>;
  using base_t::base_t;
};

template<typename FloatT>
struct generic_vec3 : base_generic_vec3<FloatT>, vec_base<generic_vec3<FloatT>>
{
  using base_t = base_generic_vec3<FloatT>;
  using base_t::base_t;
};

template<typename FloatT>
struct generic_vec4 : base_generic_vec4<FloatT>, vec_base<generic_vec4<FloatT>>
{
  using base_t = base_generic_vec4<FloatT>;
  using base_t::base_t;
};

typedef generic_vec4<float> vec4;
typedef generic_vec3<float> vec3;
typedef generic_vec2<float> vec2;

template<typename FloatT>
constexpr generic_vec3<FloatT> cross(const generic_vec3<FloatT> &a, const generic_vec3<FloatT> &b)
{
  return {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
constexpr Vec sup(const Vec& a, const Vec& b)
{
  Vec ca(a);
  ca.foreach_paired(b, [](auto& a, const auto& b){a = std::max(a, b);});
  return ca;
}

template<typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
constexpr Vec inf(const Vec& a, const Vec& b)
{
  Vec ca(a);
  ca.foreach_paired(b, [](auto& a, const auto& b){a = std::min(a, b);});
  return ca;
}

}

#endif
