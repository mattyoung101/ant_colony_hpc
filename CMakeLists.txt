# COSC3500 Ant Simulator build config
# Matt Young, 2022
cmake_minimum_required(VERSION 3.9.1)
project(ant_colony LANGUAGES C CXX)

## Compile as C++17
# getafix ships an absolutely pre-historic version of gcc that only compiles up to C++11, but my
# plan is to ship release builds as static binaries s othey can just be uploaded
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -g3)
# only supported under clang, but should be faster and allegedly better
# for mold install instructions, see: https://github.com/rui314/mold
# ---------- uncomment this if you're cool and want to compile mold, otherwise leave out ----------
#add_link_options(-fuse-ld=mold)

# Optimisation
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Release build, enabling performance and static linking")
    # compile release builds in static linking mode so we can upload them to getafix
    #
    # getafix CPU is Intel Xeon E5-2660, which should be x86-64-v2 (it's a rather old CPU)
    # see: https://unix.stackexchange.com/a/631226
    # TODO get the right mtune as well
    add_compile_options(-O3 -march=x86-64-v2 -flto)
    add_link_options(-flto)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
    message(STATUS "Profile build, enabling gprof")
    add_compile_options(-O0 -pg -flto)
    add_link_options(-flto -pg)
else()
    message(STATUS "Debug build, enabling sanitizers")
    add_compile_options(-O0 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

include_directories(include)

# libraries
# TODO must check this runs on moss!!!
include_directories(lib)

add_executable(ant_colony lib/log/log.c lib/log/log.h src/main.cpp src/world.cpp lib/stb/stb_image.c
    lib/microtar/microtar.c lib/stb/stb_image_write.c src/utils.cpp lib/tinycolor/tinycolormap.hpp
    src/pheromone.cpp lib/clip/clip.cpp lib/clip/clip_x11.cpp lib/clip/image.cpp)

find_package(Threads REQUIRED)
# TODO must check this runs on getafix (specifically xcb)
target_link_libraries(ant_colony xcb Threads::Threads)
