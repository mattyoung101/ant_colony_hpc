#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH --mem=16GB
#SBATCH --time=0-60:00
#SBATCH --mail-user=SodiumSandwich@outlook.com
#SBATCH --mail-type=ALL
#SBATCH -e ant_colony.err
#SBATCH -o ant_colony.out

# https://research.smp.uq.edu.au/computing/index.php?n=Getafix.HardwareConfiguration
# we should have up to about 192GB of RAM

module load gnu
module load openmpi3_eth/3.0.0
# CUDA if required (make sure to make it run on a GPU partition then)

# write out number of threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

date
echo ""
for i in {1..5}; do
  # make sure we use /usr/bin/time (GNU time) which supports showing max memory, etc.
  /usr/bin/time -v ./ant_colony
done