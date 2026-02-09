#ifndef VROUM3D_DEBUG_H_INCLUDED
#define VROUM3D_DEBUG_H_INCLUDED

#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <SDL2/SDL.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SPIRV-Reflect/spirv_reflect.h>

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

}

#define spvr_check(expr) if(SpvReflectResult res = expr;res != SPV_REFLECT_RESULT_SUCCESS)\
	{\
		log("spvr_check failed at ", __FILE__, ":", __LINE__);\
		log(#expr);\
		log("Value : ", res);\
		throw std::runtime_error("Vulkan error");\
	}

#define vk_check(expr) if(VkResult res = expr;res != VK_SUCCESS)\
	{\
		log("vk_check failed at ", __FILE__, ":", __LINE__);\
		log(#expr);\
		log("Value : ", string_VkResult(res));\
		throw std::runtime_error("Vulkan error");\
	}

#define sdl_check(expr) if((expr) != 0)\
	{\
		log("sdl_check failed at ", __FILE__, ":", __LINE__);\
		log(#expr);\
		log(SDL_GetError());\
		throw std::runtime_error("check error");\
	}

#define check(expr) if(!(expr))\
	{\
		log("check failed at ", __FILE__, ":", __LINE__);\
		log(#expr);\
		throw std::runtime_error("check error");\
	}

#define auto_check(expr) if constexpr(std::is_same_v<decltype(expr), VkResult>)\
{vk_check(expr)}\
else if constexpr(std::is_void_v<decltype(expr)>)\
{expr;}\
else\
{check(expr)}

#endif
