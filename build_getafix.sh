#!/bin/bash
# Attempts to build the project on getafix. Should work.
module load gnu/10.2.1
module load cmake/3.9.1
module load cuda
#rm -rf cmake-build-release-getafix
mkdir cmake-build-release-getafix
cd cmake-build-release-getafix
cmake -DCMAKE_BUILD_TYPE="Release" ..
make -j16
#cp ant_colony ..
cd ..

echo "Built successfully. Now run ./cmake-build-release-getafix/ant_colony or sbatch slurm.sh."