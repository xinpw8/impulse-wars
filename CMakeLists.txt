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

project(env
	VERSION 1.0.0
	DESCRIPTION "Impulse Wars Env"
	LANGUAGES C
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2d.git
	GIT_TAG 2c939c287bfcffcd9ae12ce34db486f6e0bd17e2 # known good commit on main
	GIT_SHALLOW FALSE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

FetchContent_Declare(
	raylib
	GIT_REPOSITORY https://github.com/raysan5/raylib.git
	GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8 # 5.5
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(env game.c)
target_include_directories(env PRIVATE include)
target_link_libraries(env PRIVATE box2d raylib)

target_compile_options(env PRIVATE "-Werror" "-Wall" "-Wpedantic" "-Wno-variadic-macros")

find_program(MOLD_EXECUTABLE "mold")
if(MOLD_EXECUTABLE)
	target_link_options(env PUBLIC "-fuse-ld=mold")
endif()
