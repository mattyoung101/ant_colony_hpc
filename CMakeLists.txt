cmake_minimum_required(VERSION 3.23)
project(ant_colony LANGUAGES C CXX)

## Compile as C++20
set(CMAKE_CXX_STANDARD 20)
#add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -g3)
# TODO depending on whether it's supported on getafix, enable lld
#add_link_options(-fuse-ld=lld)

# Optimisation
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Release build, enabling performance")
    add_compile_options(-O3 -march=native -mtune=native -flto)
    add_link_options(-flto)
else()
    message(STATUS "Debug build, enabling sanitizers")
    add_compile_options(-Og -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

include_directories(include)

# libraries
include_directories(lib)

add_executable(ant_colony src/main.cpp src/world.cpp lib/stb/stb_image.c src/tile.cpp)
