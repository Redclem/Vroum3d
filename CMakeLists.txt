cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Vroum3d)
set(CMAKE_CXX_STANDARD 20)

add_library(vroum3d STATIC
	vroum3d/core/instance.cpp
	vroum3d/core/vkutil.cpp
	vroum3d/core/pipeline.cpp
	vroum3d/core/objects.cpp
	vroum3d/core/pipeline_info.cpp
	vroum3d/gui/base.cpp
	vroum3d/gui/element.cpp
	ext/stb_impl.cpp
)

target_compile_options(vroum3d PRIVATE -Werror -Wall -Wextra)

file(GLOB examples "examples/*.cpp")

foreach(ex ${examples})
	get_filename_component(ex_name ${ex} NAME_WE)

	add_executable(${ex_name} ${ex})

	target_link_libraries(${ex_name} vroum3d SDL2 vulkan spirv-reflect-static)

	target_compile_options(${ex_name} PRIVATE -Werror -Wall -Wextra)
endforeach()

file(GLOB shaders "shaders/*")

foreach(sh ${shaders})
	get_filename_component(sh_name_we ${sh} NAME_WE)
	get_filename_component(sh_name ${sh} NAME)
	get_filename_component(sh_ext ${sh} LAST_EXT)

	if(NOT (
		(${sh_ext} STREQUAL ".vert") OR 
		(${sh_ext} STREQUAL ".frag")
	))
		continue()
	endif()
	
	add_custom_command(OUTPUT "shaders/${sh_name}.spv"
		COMMAND "glslc" "--target-env=vulkan1.4" "shaders/${sh_name}" "-o" "shaders/${sh_name}.spv"
		DEPENDS "shaders/${sh_name}"
	)

	add_custom_target(shader_${sh_name} ALL
		DEPENDS "shaders/${sh_name}.spv"
	)

	message("Adding target and command for shader : ${sh_name}")
endforeach()
