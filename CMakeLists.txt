cmake_minimum_required(VERSION 3.22)
include(FetchContent)

find_program(CLANG_EXECUTABLE "clang")
if(CLANG_EXECUTABLE)
	set(CMAKE_C_COMPILER "clang")
	set(CMAKE_CXX_COMPILER "clang++")
endif()
set(CMAKE_C_FLAGS_INIT " -std=c23")

set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined,bounds,pointer-overflow,leak")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined,bounds,pointer-overflow,leak")

set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -march=native -fno-math-errno")

project(env
	VERSION 1.0.0
	DESCRIPTION "Impulse Wars Env"
	LANGUAGES C
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2d.git
	GIT_TAG main
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

FetchContent_Declare(
	raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG master
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(env game.c)
target_include_directories(env PRIVATE include)
target_link_libraries(env PRIVATE box2d raylib)

target_compile_options(env PRIVATE "-Wall" "-Werror" "-Wpedantic" "-Wno-variadic-macros")

find_program(MOLD_EXECUTABLE "mold")
if(MOLD_EXECUTABLE)
	target_link_options(env PUBLIC "-fuse-ld=mold")
endif()
