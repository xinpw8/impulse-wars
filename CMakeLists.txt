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
	env
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
	GIT_TAG 2c939c287bfcffcd9ae12ce34db486f6e0bd17e2 # known good commit on main
	GIT_SHALLOW FALSE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

# transpile Cython to C
find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED
)

add_custom_command(
	OUTPUT env.pxd.raw
	COMMENT "Converting C headers to Cython PXD file"
	COMMAND autopxd
		-I "${CMAKE_CURRENT_SOURCE_DIR}/include"
		-I "${box2d_SOURCE_DIR}/include" 
		-I "${raylib_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/env.h" env.pxd.raw
	VERBATIM
)

# workaround for autopxd not setting bools correctly
add_custom_command(
	OUTPUT env.pxd
	COMMENT "Fixing bools in Cython PXD file"
	COMMAND "${CMAKE_COMMAND}"
		"-DSOURCE=${CMAKE_CURRENT_BINARY_DIR}/env.pxd.raw"
		"-DTARGET=${CMAKE_CURRENT_BINARY_DIR}/env.pxd"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fix-cython-bool.cmake"
	DEPENDS env.pxd.raw
	VERBATIM
)

add_custom_command(
  OUTPUT env.c
  COMMENT "Transpiling Cython to C"
  COMMAND Python::Interpreter -m cython "${CMAKE_CURRENT_SOURCE_DIR}/env.pyx" --output-file env.c
  DEPENDS env.pxd env.pyx
  VERBATIM
)

# build python module
python_add_library(env MODULE env.c WITH_SOABI)
#add_executable(env game.c)
target_include_directories(env PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} include)
target_link_libraries(env PRIVATE raylib box2d)

target_compile_options(env PRIVATE "-Werror" "-Wall" "-Wpedantic" "-Wno-variadic-macros")

find_program(MOLD_EXECUTABLE "mold")
if(MOLD_EXECUTABLE)
	target_link_options(env PUBLIC "-fuse-ld=mold")
endif()

install(TARGETS env DESTINATION .)
