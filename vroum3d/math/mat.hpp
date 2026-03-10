#ifndef VROUM3D_MATH_MAT_HPP_INCLUDED
#define VROUM3D_MATH_MAT_HPP_INCLUDED

#include "vec.hpp"
#include <cstddef>
#include <numeric>
#include <ostream>
#include <type_traits>

namespace Vroum3d::Math
{

template<typename FloatT, std::size_t N>
struct mat
{
  using float_t = FloatT;
  constexpr static auto n = N;

  float_t data[n][n];

  template<typename Mat>
  class Acc
  {
    friend mat;
    mat& m;
    std::size_t j;

    Acc(mat& _m, std::size_t _j) : m(_m), j(_j) {}
  public:
    auto& operator[](auto i) {return m.at(i,j);}
  };

  // Access with mathematical notation (row, col)
  Acc<mat> operator[](auto idx) {return {*this, idx};}
  Acc<const mat> operator[](auto idx) const {return {*this, idx};}


  float_t& at(auto i, auto j) {return data[j][i];}
  const float_t& at(auto i, auto j) const {return data[j][i];}

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
            std::is_same_v<typename Vec::float_t, float_t> &&
            n == Vec::n_comp(), bool> = true>
  static mat scale(const Vec& vec)
  {
    mat res = zero();

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

  mat& operator+=(const mat& rhs) const
  {
    foreach([&](auto& val, auto i, auto j) {
      val += rhs[i][j];
    });
  }

  mat operator+(const mat& rhs) const
  {
    return mat(*this) += rhs;
  }

  mat& operator*=(const mat& rhs) const
  {
    return operator=(*this * rhs);
  }

  mat& operator*=(auto scal) const
  {
    foreach([&](auto& val) {
      val *= scal;
    });
  }

  mat operator*(auto scal) const
  {
    return mat(*this) *= scal;
  }
  

  template<typename Vec, std::enable_if_t<is_vec<Vec> &&
            std::is_same_v<typename Vec::float_t, float_t>, bool> = true>
  Vec operator*(const Vec& vec)
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
    s << "[";
    
    foreach([&](const auto& val, auto, auto j){
      if(!j) s << '\n';
      s << "\t" << val;
    });
    s << "\n]";
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

typedef mat<float, 4> mat4;
typedef mat<float, 3> mat3;
typedef mat<float, 2> mat2;

}

#endif 
