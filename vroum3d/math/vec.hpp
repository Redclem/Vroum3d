#ifndef VROUM3D_MATH_VEC_HPP_INCLUDED
#define VROUM3D_MATH_VEC_HPP_INCLUDED

#include <algorithm>
#include <functional>
#include <ostream>
#include <cmath>
#include <type_traits>
#include <cstdint>

namespace Vroum3d::Math
{

struct vec_root {};

template<typename T>
constexpr bool is_vec = std::is_base_of_v<vec_root, T>;

template<typename Deriv>
struct vec_base : vec_root, Deriv
{
  using deriv_t = Deriv;

  using deriv_t::deriv_t;
  using scal_t = deriv_t::scal_t;

  template<typename Scal>
  static constexpr bool scal_arith_compat = std::is_convertible_v<Scal, scal_t>;

  template<typename T>
  static constexpr bool arith_compat = is_vec<T> || scal_arith_compat<T>;

  template<typename ... Args, std::enable_if_t<std::is_constructible_v<deriv_t, Args&&...>, bool> = true>
  constexpr vec_base(Args&& ... ags) : deriv_t(std::forward<Args>(ags)...) {}

  constexpr vec_base(const vec_base& from) = default;
  constexpr vec_base& operator=(const vec_base& rhs) = default;

  template<typename FromVec, std::enable_if_t<is_vec<FromVec>, bool> = true>
  constexpr vec_base(const FromVec& from)
  {
    auto arr = from.to_array();
    auto iter = arr.begin();
    drv().foreach([&](auto& val) {val = *(iter++);});
  }

  template<typename FromVec, std::enable_if_t<is_vec<FromVec>, bool> = true>
  constexpr vec_base& operator=(const FromVec& rhs)
  {
    auto arr = rhs.to_array();
    auto iter = arr.begin();
    drv().foreach([&](auto& val) {val = *(iter++);});
    return *this;
  }

  constexpr static std::size_t n_comp()
  {
    std::size_t nc(0);
    deriv_t().foreach([&](auto){nc++;});
    return nc;
  }

  constexpr static std::size_t c_n_comp = n_comp();

  template<typename Deriv2>
  constexpr vec_base(const vec_base<Deriv2>& from)
  {
    auto arr = from.to_array();
    auto iter = arr.begin();

    drv().foreach([&](auto& val) {val = iter == arr.end() ? 0.0 : *(iter++);});
  }

  constexpr auto& drv() {return *this;}
  constexpr const auto& drv() const {return *this;}

  template<typename Vec, std::enable_if_t<is_vec<Vec> && c_n_comp == Vec::c_n_comp, bool> = true>
  constexpr vec_base& asg_add(const Vec& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, const auto& b){a += b;});
    return *this;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && c_n_comp == Vec::c_n_comp, bool> = true>
  constexpr vec_base& asg_min(const Vec& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, const auto& b){a -= b;});
    return *this;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && c_n_comp == Vec::c_n_comp, bool> = true>
  constexpr vec_base& asg_div(const Vec& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, auto b){a /= b;});
    return *this;
  }

  template<typename Vec, std::enable_if_t<is_vec<Vec> && c_n_comp == Vec::c_n_comp, bool> = true>
  constexpr vec_base& asg_mul(const Vec& rhs)
  {
    drv().foreach_paired(rhs.drv(), [&](auto& a, auto b){a *= b;});
    return *this;
  }

  template<typename Scal, std::enable_if_t<scal_arith_compat<Scal>, bool> = true>
  constexpr vec_base& asg_add(Scal rhs)
  {
    drv().foreach([&](auto& a){a += rhs;});
    return *this;
  }

  template<typename Scal, std::enable_if_t<scal_arith_compat<Scal>, bool> = true>
  constexpr vec_base& asg_min(Scal rhs)
  {
    drv().foreach([&](auto& a){a -= rhs;});
    return *this;
  }

  template<typename Scal, std::enable_if_t<scal_arith_compat<Scal>, bool> = true>
  constexpr vec_base& asg_mul(Scal scal)
  {
    drv().foreach([&](auto& a){a *= scal;});
    return *this;
  }

  template<typename Scal, std::enable_if_t<scal_arith_compat<Scal>, bool> = true>
  constexpr vec_base& asg_div(Scal scal)
  {
    drv().foreach([&](auto& a){a /= scal;});
    return *this;
  }

  constexpr vec_base& negate() 
  {
    drv().foreach([](auto& val) {val = -val;});
    return *this;
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base& operator+=(const Rhs& rhs)
  {
    return asg_add(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base& operator-=(const Rhs& rhs)
  {
    return asg_min(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base& operator*=(const Rhs& rhs)
  {
    return asg_mul(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base& operator/=(const Rhs& rhs)
  {
    return asg_div(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base operator+(Rhs rhs) const
  {
    return copy().asg_add(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base operator-(Rhs rhs) const
  {
    return copy().asg_min(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base operator*(Rhs rhs) const
  {
    return copy().asg_mul(rhs);
  }

  template<typename Rhs, std::enable_if_t<arith_compat<Rhs>, bool> = true>
  constexpr vec_base operator/(Rhs rhs) const
  {
    return copy().asg_div(rhs);
  }

  constexpr vec_base operator-() const
  {
    return copy().negate();
  }

  constexpr auto copy() const {return vec_base(*this);}

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
    scal_t res(0);
    drv().foreach_paired(rhs.drv(), [&](const auto& a, const auto& b) {res += a * b;});

    return res;
  }

  constexpr auto norm2() const
  {
    scal_t res(0);
    drv().foreach([&](const auto& a) {res += a * a;});

    return res;
  }

  constexpr auto norm() const
  {
    return std::sqrt(norm2());
  }

  constexpr auto to_array() const
  {
    std::array<scal_t, c_n_comp> res;
    auto iter = res.begin();

    drv().foreach([&](auto val){*(iter++) = val;});
    return res;
  }

  constexpr operator auto () const
  {
    std::array<scal_t, c_n_comp> res;
    auto iter = res.begin();
    drv().foreach([&](const auto& val){*iter = val;});
    return res;
  }

  constexpr auto max_coord() const
  {
    scal_t res;
    drv().foreach([&](auto val) {res = val;});
    drv().foreach([&](auto val) {res = std::max(res, val);});
    return res;
  }

  constexpr auto min_coord() const
  {
    scal_t res;
    drv().foreach([&](auto val) {res = val;});
    drv().foreach([&](auto val) {res = std::min(res, val);});
    return res;
  }

  constexpr auto& clamp(auto min, auto max)
  {
    drv().foreach([&](auto& val) {val = std::clamp<scal_t>(val, min, max);});
    return *this;
  }

  constexpr auto abs() const
  {
    vec_base copy(*this);
    copy.foreach([](auto & val) {val = std::abs(val);});      
    return copy;
  }

  template<std::size_t idx>
  constexpr const auto& get() const
  {
    const typename Deriv::scal_t* ptr_res;
    std::size_t n_elem = 0;
    drv().foreach([&](const auto& val) {if(n_elem++ == idx) ptr_res = &val;});
    return *ptr_res;
  }

  template<std::size_t idx>
  constexpr auto& get()
  {
    typename Deriv::scal_t* ptr_res;
    std::size_t n_elem = 0;
    drv().foreach([&](auto& val) {if(n_elem++ == idx) ptr_res = &val;});
    return *ptr_res;
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

template<typename Scal, typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
constexpr auto operator*(Scal lhs, const Vec& rhs)
{
  return rhs.copy().asg_mul(lhs);
}

template<typename Scal, typename Vec, std::enable_if_t<is_vec<Vec>, bool> = true>
constexpr auto operator+(Scal lhs, const Vec& rhs)
{
  return rhs.copy().asg_add(lhs);
}

template<typename ScalT>
struct base_generic_vec2
{
  using scal_t = ScalT;

  union{
    scal_t x;
    scal_t r;
  };

  union{
    scal_t y;
    scal_t g;
  };

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

  constexpr base_generic_vec2(scal_t _x, scal_t _y) : x(_x), y(_y) {}
  constexpr base_generic_vec2(scal_t scal = 0) : x(scal), y(scal) {}
};

template<typename ScalT>
struct base_generic_vec3 : base_generic_vec2<ScalT>
{
  using base_t = base_generic_vec2<ScalT>;
  using typename base_t::scal_t;

  union{
    scal_t z;
    scal_t b;
  };

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

  constexpr base_generic_vec3(scal_t _x, scal_t _y, scal_t _z) : base_t(_x, _y), z(_z) {}
  constexpr base_generic_vec3(scal_t scal = 0) : base_t(scal), z(scal) {}

};

template<typename ScalT>
struct base_generic_vec4 : base_generic_vec3<ScalT>
{
  using base_t = base_generic_vec3<ScalT>;
  using typename base_t::scal_t;
    
  union{
    scal_t w;
    scal_t a;
  };

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

  constexpr base_generic_vec4(scal_t _x, scal_t _y, scal_t _z, scal_t _w) : base_t(_x, _y, _z), w(_w) {}
  constexpr base_generic_vec4(scal_t scal = 0) : base_t(scal), w(scal) {}

};

template<typename ScalT>
using generic_vec2 = vec_base<base_generic_vec2<ScalT>>;

template<typename ScalT>
using generic_vec3 = vec_base<base_generic_vec3<ScalT>>;

template<typename ScalT>
using generic_vec4 = vec_base<base_generic_vec4<ScalT>>;

typedef generic_vec4<float> vec4;
typedef generic_vec3<float> vec3;
typedef generic_vec2<float> vec2;

typedef generic_vec4<double> dvec4;
typedef generic_vec3<double> dvec3;
typedef generic_vec2<double> dvec2;

typedef generic_vec4<std::int32_t> ivec4;
typedef generic_vec3<std::int32_t> ivec3;
typedef generic_vec2<std::int32_t> ivec2;

typedef generic_vec4<std::uint32_t> uvec4;
typedef generic_vec3<std::uint32_t> uvec3;
typedef generic_vec2<std::uint32_t> uvec2;

template<typename ScalT>
constexpr generic_vec3<ScalT> cross(const generic_vec3<ScalT> &a, const generic_vec3<ScalT> &b)
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

template<typename Scal, typename Vec, std::enable_if_t<!is_vec<Scal> && is_vec<Vec>, bool> = true>
constexpr Vec operator/(Scal scal, const Vec& v)
{
  Vec res;
  res.foreach_paired(v, [&](auto& res, const auto& val) {res = scal / val;});
  return res;
}

}

namespace std
{

template<typename Drv>
struct tuple_size<Vroum3d::Math::vec_base<Drv>> : integral_constant<size_t, Vroum3d::Math::vec_base<Drv>::c_n_comp> {};

template<size_t I, typename Drv> 
struct tuple_element<I, Vroum3d::Math::vec_base<Drv>> {
    using type = typename Vroum3d::Math::vec_base<Drv>::scal_t;
};

}


#endif
