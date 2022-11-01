#!/bin/bash
# Attempts to build the project on getafix. Only designed to be used once and directly out of the box, to prove the project compiles.
# If repeating this script multiple times, more care should be taken (in regards to CMake in particular).
module load gnu/10.2.1
module load cmake/3.9.1
module load cuda

mkdir cmake-build-release-getafix
cd cmake-build-release-getafix
../cmake-3.24.2-linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE="Release" ..
make -j16
cd ..

echo "Built successfully. Now run ./cmake-build-release-getafix/ant_colony or sbatch slurm.sh."
