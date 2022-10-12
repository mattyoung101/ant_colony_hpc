# COSC3500 Ant Simulator build config
# Matt Young, 2022
cmake_minimum_required(VERSION 3.9.1)
project(ant_colony LANGUAGES C CXX)

## Compile as C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -g3)

if (${CMAKE_VERSION} VERSION_LESS "3.16.0")
    message(STATUS "Ancient CMake version detected, doing our best")
    # if they forget to pass CMAKE_BUILD_TYPE=Release, compilation won't work, so remind them
    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
        message(FATAL_ERROR "Only CMAKE_BUILD_TYPE=Release is supported on getafix. Please see README.md.")
    endif()
endif()

# Optimisation
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Release build, enabling performance")
    # since we're no longer doing the whole static linking thing, just do march=native and mtune=native
    # should compile the best on both my dev machine and getafix
    add_compile_options(-O3 -march=native -mtune=native -flto -fno-math-errno -freciprocal-math)

    # extremely annoyingly the **latest** version of CMake getafix ships is 3.9.1 which doesn't even support
    # the add_link_options command
    if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
        message(STATUS "Using old link options method")
        set (CMAKE_SHARED_LINKER_FLAGS "-flto -fno-math-errno")
    else()
        message(STATUS "Using new link options method")
        add_link_options(-flto -fno-math-errno)
    endif()
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
    message(STATUS "Profile build, enabling gprof")
    # TODO enable gprof here if you want it
    add_compile_options(-O0)
    add_link_options()
else()
    message(STATUS "Debug build, enabling sanitizers")
    add_compile_options(-O0 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

include_directories(include)
include_directories(lib)

add_executable(ant_colony lib/log/log.c lib/log/log.h src/main.cpp src/world.cpp lib/stb/stb_image.c
    lib/microtar/microtar.c lib/stb/stb_image_write.c src/utils.cpp lib/tinycolor/tinycolormap.hpp
    src/pheromone.cpp lib/clip/clip.cpp lib/clip/clip_x11.cpp lib/clip/image.cpp)

add_executable(dump_random src/dump_random.cpp lib/log/log.c lib/log/log.h src/utils.cpp)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(ant_colony xcb Threads::Threads OpenMP::OpenMP_CXX)
