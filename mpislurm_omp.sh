#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=20
#SBATCH --mem=32GB
#SBATCH --time=0-480:00
#SBATCH -e log_%j.err
#SBATCH -o log_%j.out

# Slurm job to compare the OpenMP version against the MPI version of the colony

module load gnu

# write out number of threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

date
mpiexec ./cmake-build-release-getafix/ant_colony antconfig_megamap_mpi.ini