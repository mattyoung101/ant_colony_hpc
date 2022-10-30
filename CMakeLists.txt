# COSC3500 Ant Simulator build config
# Matt Young, 2022
cmake_minimum_required(VERSION 3.9.1)
project(ant_colony LANGUAGES C CXX)

## Compile as C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -g3)

if (${CMAKE_VERSION} VERSION_LESS "3.16.0")
    message(STATUS "Ancient CMake version detected (this is probably getafix)")
    # if they forget to pass CMAKE_BUILD_TYPE=Release, compilation won't work, so remind them
    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
        message(WARNING "Non-Release builds will not work with default getafix CMake. You probably want
        to add -DCMAKE_BUILD_TYPE=Release. See README.md for more details.")
    endif()
endif()

# Optimisation
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Release build, enabling performance")
    # since we're no longer doing the whole static linking thing, just do march=native and mtune=native
    # should compile the best on both my dev machine and getafix
    # add -fno-math-errno and -freciprocal-math for some negligible speedups hopefully
    add_compile_options(-O3 -march=native -mtune=native -flto -fno-math-errno -freciprocal-math)

    # getafix ships an ancient version of CMake. we have to at least support the Release target on its
    # ancient old version, so detect if we need to use the old method or the new one.
    if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
        message(STATUS "Using old link options method")
        set (CMAKE_SHARED_LINKER_FLAGS "-flto -fno-math-errno")
    else()
        message(STATUS "Using new link options method")
        add_link_options(-flto -fno-math-errno)
    endif()
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
    message(STATUS "Profile build, enabling gprof")
    # this one is for thread sanitizer
#    add_compile_options(-O0 -fsanitize=thread -fno-omit-frame-pointer)
#    add_link_options(-fsanitize=thread)

    # this one is for normal profiling
    add_compile_options(-O0)
else()
    message(STATUS "Debug build, enabling sanitizers")
    # enable ASan and UBSan, no optimisation
    add_compile_options(-O0 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

include_directories(include)
include_directories(lib)

add_executable(ant_colony lib/log/log.c lib/log/log.h src/main.cpp src/world.cpp lib/stb/stb_image.c
    lib/microtar/microtar.c lib/stb/stb_image_write.c src/utils.cpp lib/tinycolor/tinycolormap.hpp
    src/pheromone.cpp lib/clip/clip.cpp lib/clip/clip_x11.cpp lib/clip/image.cpp include/ants/snapgrid.h
    include/ants/defines.h)

add_executable(dump_random src/dump_random.cpp lib/log/log.c lib/log/log.h src/utils.cpp)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(ant_colony xcb Threads::Threads OpenMP::OpenMP_CXX)
