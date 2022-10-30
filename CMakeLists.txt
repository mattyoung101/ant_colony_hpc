# COSC3500 Ant Simulator build config
# Matt Young, 2022
cmake_minimum_required(VERSION 3.16.0)
project(ant_colony LANGUAGES C CXX)

# see if CUDA is available
include(CheckLanguage)
check_language(CUDA)

## Compile as C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -g3)

# Optimisation
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Release build, enabling performance")
    # since we're no longer doing the whole static linking thing, just do march=native and mtune=native
    # should compile the best on both my dev machine and getafix
    # add -fno-math-errno and -freciprocal-math for some negligible speedups hopefully
    add_compile_options(-O3 -march=native -mtune=native -flto -fno-math-errno -freciprocal-math)
    add_link_options(-flto -fno-math-errno)
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

# CUDA for ant_colony target
if (CMAKE_CUDA_COMPILER)
    message(STATUS "Detected CUDA")
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 11)
    set_target_properties(ant_colony PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    add_compile_definitions(HAVE_CUDA)
    # https://forums.developer.nvidia.com/t/passing-flags-to-nvcc-via-cmake/75768/4
    target_compile_options(ant_colony PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:
        -O2 --gpu-architecture=sm_35 -Wno-deprecated-gpu-targets -std=c++11 -g
        >)
else()
    message(STATUS "No CUDA detected")
endif()

add_executable(dump_random src/dump_random.cpp lib/log/log.c lib/log/log.h src/utils.cpp)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(ant_colony xcb Threads::Threads OpenMP::OpenMP_CXX)
