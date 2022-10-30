#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=96GB
#SBATCH --time=0-60:00
#SBATCH -e log_%j.err
#SBATCH -o log_%j.out

# https://research.smp.uq.edu.au/computing/index.php?n=Getafix.HardwareConfiguration
# we should have up to about 192GB of RAM

# [you can enable this] --mail-user=SodiumSandwich@outlook.com
# [you can enable this] --mail-type=ALL

module load gnu
module load openmpi3_eth/3.0.0
# CUDA if required (make sure to make it run on a GPU partition then)

# write out number of threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

for i in {1..2}; do
  date
  echo "========== Run $i ==========="
  # make sure we use /usr/bin/time (GNU time) which supports showing max memory, etc.
  /usr/bin/time -v ./cmake-build-release-getafix/ant_colony
  # valgrind ./cmake-build-release-getafix/ant_colony
  echo
done