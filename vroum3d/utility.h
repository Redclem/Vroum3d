#ifndef VROUM3D_UTILITY_H_INCLUDED
#define VROUM3D_UTILITY_H_INCLUDED

#include "debug.h"

#include <volk.h>

#include <utility>
#include <vector>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <vulkan/vulkan_core.h>

namespace Vroum3d
{

class NonCopyable
{
public:
	constexpr NonCopyable() {}

	constexpr NonCopyable(const NonCopyable& from) = delete;
	constexpr NonCopyable& operator=(const NonCopyable& rhs) = delete;
};

template<typename T>
struct AssignDestroy : public NonCopyable
{
	constexpr AssignDestroy() = default;

	constexpr AssignDestroy(AssignDestroy&&) {};

	constexpr AssignDestroy& operator=(AssignDestroy&&)
	{
		static_cast<T*>(this)->destroy();
		return *this;
	}
};

template<typename T, T Def = 0>
class Handle : public NonCopyable
{
	T m_hdl = def;
public:
	constexpr static auto def = Def;

	constexpr Handle() : m_hdl(def) {}
	constexpr Handle(T val) : m_hdl(val) {}
	constexpr Handle(Handle&& rhs) : m_hdl(std::exchange(rhs.m_hdl, def)) {}

	constexpr Handle& operator=(Handle&& rhs)
	{
		m_hdl = std::exchange(rhs.m_hdl, def);
		return *this;
	}

	constexpr Handle& operator=(T val)
	{
		m_hdl = val;
		return *this;
	}
	
	constexpr const T& get() const {return m_hdl;}

	constexpr operator const T&() const {return get();}
	constexpr auto operator&() {return &m_hdl;} /** Not really useful according to C++ standard */
	constexpr auto operator&() const {return &m_hdl;} /** Not really useful according to C++ standard */

	constexpr const auto& operator*() const {return *m_hdl;}
	constexpr auto& operator*() {return *m_hdl;}
	constexpr auto operator->() const {return m_hdl;}
	constexpr auto operator->() {return m_hdl;}

	template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun, T&>, bool> = true>
	void destroy_with(Fun&& fun)
	{
		if(m_hdl != def)
		{
			fun(m_hdl);
			m_hdl = def;
		}
	}

	template<typename Fun, std::enable_if_t<std::is_invocable_v<Fun>, bool> = true>
	void destroy_with(Fun&& fun)
	{
		if(m_hdl != def)
		{
			fun();
			m_hdl = def;
		}
	}
};

template<typename T>
using VkHandle = Handle<T, VK_NULL_HANDLE>;

namespace enumerate
{

	template <typename T>
	struct res {
		constexpr static bool is_enum_fun = false;
	};

	template<typename R, typename ... Args>
	struct res<R(*)(Args...)>
	{
		constexpr static bool is_enum_fun = true;
		constexpr static bool void_res = std::is_void_v<R>;
		using result_t = std::remove_pointer_t<typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>;
	};
}

template<typename Func, typename ... Args,
	std::enable_if_t<!enumerate::res<Func>::void_res, bool> = true>
std::vector<typename enumerate::res<Func>::result_t> wrap_enumerate(Func f,Args&&...ags)
{
	static_assert(enumerate::res<Func>::is_enum_fun, "This function must be called with a Vulkan enumerator function");

	using res = typename enumerate::res<Func>::result_t;
	std::uint32_t size;
	
	auto_check(f(ags..., &size, nullptr));

	std::vector<res> vec(size);

	auto_check(f(ags..., &size, vec.data()));

	return vec;
}


template<typename Func, typename ... Args,
	std::enable_if_t<enumerate::res<Func>::void_res, bool> = true>
std::vector<typename enumerate::res<Func>::result_t> wrap_enumerate(Func f, Args&&...ags)
{
	static_assert(enumerate::res<Func>::is_enum_fun, "This function must be called with a Vulkan enumerator function");

	using res = typename enumerate::res<Func>::result_t;
	std::uint32_t size;
	
	f(ags..., &size, nullptr);

	std::vector<res> vec(size);

	f(ags..., &size, vec.data());

	return vec;

}

}

#endif
