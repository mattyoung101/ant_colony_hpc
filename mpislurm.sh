#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --ntasks=40
#SBATCH --ntasks-per-node=10
#SBATCH --cpus-per-task=1
#SBATCH --mem=256GB
#SBATCH --time=0-480:00
#SBATCH -e log_%j.err
#SBATCH -o log_%j.out

# Slurm job for the MPI version of the ant colony

module load gnu
module load openmpi3_eth/3.0.0

date
mpiexec ./cmake-build-release-getafix/ant_colony antconfig_megamap_mpi.ini