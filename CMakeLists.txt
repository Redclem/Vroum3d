cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Vroum3d)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
include(CTest)

enable_testing()

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)

FetchContent_Declare(
  spirv_reflect
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect
mingw-w64-ucrt-x86_64-neovim
  GIT_TAG main
)

FetchContent_MakeAvailable(spirv_reflect)

set(VULKAN_LIB "vulkan")

if(${MSYS})
	message("Setting up for MSYS")
	set(VULKAN_LIB "vulkan-1")
	set(EXEC_LINK_OPTS "-mconsole")
	add_compile_definitions(MSYS)
endif()

add_library(vroum3d STATIC
	vroum3d/core/instance.cpp
	vroum3d/core/vkutil.cpp
	vroum3d/core/pipeline.cpp
	vroum3d/core/objects.cpp
	vroum3d/core/pipeline_info.cpp
  	vroum3d/core/allocator.cpp
	vroum3d/gui/base.cpp
	vroum3d/gui/element.cpp
	ext/stb_impl.cpp
)

set(SHADERS_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders/")
set(SHADERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders/")

make_directory("${SHADERS_BINARY_DIR}")


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(VROUM3D_CXXFLAGS -Wall -Werror -Wextra)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(VROUM3D_CXXFLAGS /WX /Wall)
endif()

target_compile_options(vroum3d PRIVATE "${VROUM3D_CXXFLAGS}")
target_link_libraries(vroum3d PUBLIC SDL2main SDL2 ${VULKAN_LIB} spirv-reflect-static)
target_include_directories(vroum3d INTERFACE vroum3d)

file(GLOB examples "examples/*.cpp")

function(add_example ex_name)

	add_executable(${ex_name} "examples/${ex_name}.cpp")

	target_link_libraries(${ex_name} vroum3d)

	target_link_options(${ex_name} PRIVATE ${EXEC_LINK_OPTS})

	target_compile_options(${ex_name} PRIVATE "${VROUM3D_CXXFLAGS}")
endfunction()

add_example(gui)
add_example(empty_pipeline)
add_example(instance)
add_example(pipeman)
add_example(simple_pipeline)
add_example(static)
add_example(bag_simple)

function(add_exec_test test_name)

	add_executable(test_${test_name} "tests/${test_name}.cpp")

	target_link_libraries(test_${test_name} vroum3d)

	target_link_options(test_${test_name} PRIVATE ${EXEC_LINK_OPTS})

	target_compile_options(test_${test_name} PRIVATE "${VROUM3D_CXXFLAGS}")

	add_test(NAME ${test_name} COMMAND test_${test_name})
endfunction()

add_exec_test(allocator_log)
add_exec_test(allocator)
add_exec_test(allocator_nooverlap)
add_exec_test(allocator_fsb)

file(GLOB shaders "shaders/*")

function (add_shader sh)
	get_filename_component(sh_name_we ${sh} NAME_WE)
	get_filename_component(sh_ext ${sh} LAST_EXT)

	if(NOT (
		(${sh_ext} STREQUAL ".vert") OR
		(${sh_ext} STREQUAL ".frag")
	))
		continue()
	endif()

	add_custom_command(OUTPUT "${SHADERS_BINARY_DIR}/${sh}.spv"
		COMMAND "glslc" "--target-env=vulkan1.4" "${SHADERS_SOURCE_DIR}/${sh}" "-o" "${SHADERS_BINARY_DIR}/${sh}.spv"
		DEPENDS "${SHADERS_SOURCE_DIR}/${sh}"
	)

	add_custom_target(shader_${sh} ALL
		DEPENDS "${SHADERS_BINARY_DIR}/${sh}.spv"
	)
endfunction()

add_shader(empty.frag)
add_shader(empty.vert)
add_shader(simple.frag)
add_shader(simple.vert)
add_shader(static.frag)
add_shader(static.vert)
