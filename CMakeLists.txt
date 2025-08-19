cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Vroum3d)
set(CMAKE_CXX_STANDARD 20)

add_library(vroum3d STATIC
	vroum3d/core/instance.cpp
	vroum3d/core/vkutil.cpp
	ext/stb_impl.cpp
)

file(GLOB examples "examples/*.cpp")

target_compile_options(vroum3d PRIVATE -Werror -Wall -Wextra)

foreach(ex ${examples})
	get_filename_component(ex_name ${ex} NAME_WE)

	add_executable(${ex_name} ${ex})

	target_link_libraries(${ex_name} vroum3d SDL2 vulkan)

	target_compile_options(${ex_name} PRIVATE -Werror -Wall -Wextra)
endforeach()
