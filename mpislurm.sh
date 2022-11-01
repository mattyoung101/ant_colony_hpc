#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32GB
#SBATCH --time=0-480:00
#SBATCH -e log_%j.err
#SBATCH -o log_%j.out
#SBATCH --mail-user=SodiumSandwich@outlook.com
#SBATCH --mail-type=ALL
# TODO make this MPI compatible

# Slurm job for the MPI version of the ant colony

# https://research.smp.uq.edu.au/computing/index.php?n=Getafix.HardwareConfiguration
# we should have up to about 192GB of RAM

module load gnu
module load openmpi3_eth/3.0.0

# write out number of threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

date
mpiexec ./cmake-build-release-getafix/ant_colony