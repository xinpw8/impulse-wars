cmake_minimum_required(VERSION 3.22)
include(FetchContent)

find_program(CLANG_EXECUTABLE "clang")
if(CLANG_EXECUTABLE)
	set(CMAKE_C_COMPILER "clang")
	set(CMAKE_CXX_COMPILER "clang++")
endif()
set(CMAKE_C_FLAGS_INIT " -std=c2x")

set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined,bounds,pointer-overflow,leak")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined,bounds,pointer-overflow,leak")

set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -march=native -fno-math-errno")

project(
	impulse-wars
	DESCRIPTION "Impulse Wars Env"
	LANGUAGES C
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# fetch and configure dependencies
FetchContent_Declare(
	raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8 # 5.5
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

# if box2d is fetched first installing built python module will fail
# for reasons unbeknownst to mere mortals
# maybe due to install prefix schenanigans?
FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2d.git
	GIT_TAG f0763aca7d4dfa8f1c19c2c4a9e66fa6e93ea32e # known good commit on main
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

function(configure_target target_name)
	target_include_directories(
		${target_name} PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
	)
	target_link_libraries(${target_name} PRIVATE raylib box2d)

	target_compile_options(${target_name} PRIVATE "-Werror" "-Wall" "-Wpedantic" "-Wno-variadic-macros")

	find_program(MOLD_EXECUTABLE "mold")
	if(MOLD_EXECUTABLE)
		target_link_options(${target_name} PUBLIC "-fuse-ld=mold")
	endif()
endfunction()

if(DEFINED BUILD_PYTHON_MODULE)
	# transpile Cython to C
	find_package(
		Python
		COMPONENTS Interpreter Development.Module
		REQUIRED
	)

	add_custom_command(
		OUTPUT impulse_wars.pxd.raw
		COMMENT "Converting C headers to Cython PXD file"
		COMMAND autopxd
			-I "${CMAKE_CURRENT_SOURCE_DIR}/src"
			-I "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
			-I "${box2d_SOURCE_DIR}/include" 
			-I "${raylib_SOURCE_DIR}"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/env.h" impulse_wars.pxd.raw
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/env.h"
		VERBATIM
	)

	# workaround for autopxd not setting bools correctly
	add_custom_command(
		OUTPUT impulse_wars.pxd
		COMMENT "Fixing bools in Cython PXD file"
		COMMAND "${CMAKE_COMMAND}"
			"-DSOURCE=${CMAKE_CURRENT_BINARY_DIR}/impulse_wars.pxd.raw"
			"-DTARGET=${CMAKE_CURRENT_BINARY_DIR}/impulse_wars.pxd"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fix-cython-bool.cmake"
		DEPENDS impulse_wars.pxd.raw
		VERBATIM
	)

	# copy Cython file to binary dir to avoid "class is declared but not defined" errors
	# apparently Cython doesn't like when you aren't compiling a file in the working directory
	# and that file imports a PXD file
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cy_impulse_wars.pyx"
		"${CMAKE_CURRENT_BINARY_DIR}/cy_impulse_wars.pyx"
		COPYONLY
	)

	add_custom_command(
		OUTPUT impulse_wars.c
		COMMENT "Transpiling Cython to C"
		COMMAND Python::Interpreter -m cython
			-3
			-X boundscheck=false
			-X initializedcheck=false
			-X wraparound=false
			-X cdivision=true
			cy_impulse_wars.pyx -o impulse_wars.c
		DEPENDS impulse_wars.pxd cy_impulse_wars.pyx
		VERBATIM
	)

	# build python module
	python_add_library(cy_impulse_wars MODULE impulse_wars.c WITH_SOABI)
	configure_target(cy_impulse_wars)
	install(TARGETS cy_impulse_wars DESTINATION .)
elseif(DEFINED BUILD_DEMO)
	add_executable(demo "${CMAKE_CURRENT_SOURCE_DIR}/src/demo.c")
	configure_target(demo)
elseif(DEFINED BUILD_BENCHMARK)
	add_executable(benchmark "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark.c")
	configure_target(benchmark)
endif()
