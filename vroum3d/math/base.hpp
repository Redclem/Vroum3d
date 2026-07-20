#ifndef VROUM3D_MATH_BASE_HPP_INCLUDED
#define VROUM3D_MATH_BASE_HPP_INCLUDED

#include <type_traits>

namespace Vroum3d::Math
{

struct vec_root {};

template<typename T>
constexpr bool is_vec = std::is_base_of_v<vec_root, T>;

struct mat_root {
	bool operator==(const mat_root& rhs) const = default;
	bool operator!=(const mat_root& rhs) const = default;
};

template<typename Mat>
constexpr bool is_mat = std::is_base_of_v<mat_root, Mat>;

template<typename T>
constexpr bool is_vec_or_mat = is_mat<T> || is_vec<T>;

}

#endif