#ifndef VROUM3D_DEBUG_H_INCLUDED
#define VROUM3D_DEBUG_H_INCLUDED

#include <iostream>
#include <source_location>
#include <stdexcept>
#include <type_traits>

#include <SDL2/SDL.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan_core.h>

namespace Vroum3d
{
#ifdef NDEBUG
	constexpr bool debug = false;
#else
	constexpr bool debug = true;
#endif

template<bool enable = debug, typename Ag1, typename ... Args>
void log(Ag1 && ag, Args && ... args)
{
	if constexpr (enable)
	{
		std::cout << std::forward<Ag1>(ag);

		if constexpr (sizeof...(Args))
			log(std::forward<Args>(args)...);
		else
			std::cout << '\n';
	}
}

inline void spvr_check(SpvReflectResult res, std::source_location loc = std::source_location::current())
{
	if(res != SPV_REFLECT_RESULT_SUCCESS)
	{
		log("spvr_check failed at ", loc.file_name(), ":", loc.line(), " col ", loc.column());
		log("In function ", loc.function_name());
		log("Value : ", res);
		throw std::runtime_error("SpvReflect error");
	}
}

inline void vk_check(VkResult res, std::source_location loc = std::source_location::current())
{
	if(res != VK_SUCCESS)
	{
		log("vk_check failed at ", loc.file_name(), ":", loc.line(), " col ", loc.column());
		log("In function ", loc.function_name());
		log("Value : ", string_VkResult(res));
		throw std::runtime_error("Vulkan error");
	}
}

inline void sdl_check(int res, std::source_location loc = std::source_location::current())
{
	if(res != 0)
	{
		log("sdl_check failed at ", loc.file_name(), ":", loc.line(), " col ", loc.column());
		log("In function ", loc.function_name());
		log(SDL_GetError());
		throw std::runtime_error("check error");
	}
}


inline void check(bool res, std::source_location loc = std::source_location::current())
{
	if(!res)
	{
		log("check failed at ", loc.file_name(), ":", loc.line(), " col ", loc.column());
		log("In function ", loc.function_name());
		throw std::runtime_error("check error");
	}
}

template<typename Res>
inline void auto_check(Res res, std::source_location loc = std::source_location::current())
{
	check(res, loc);
}

template<>
inline void auto_check<VkResult>(VkResult res, std::source_location loc)
{
	vk_check(res, loc);
}

}

#endif
