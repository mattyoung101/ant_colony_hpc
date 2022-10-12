#!/bin/bash -l
#
# TODO improve these config options so it will work better with OpenMP (see open tabs)
#SBATCH --job-name=antsim
#SBATCH --nodes=1
#SBATCH --ntasks=2
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=5G
#SBATCH --time=0-2:00
#SBATCH --mail-user=SodiumSandwich@outlook.com
#SBATCH --mail-type=ALL

module load gnu
module load openmpi3_eth/3.0.0
# cuda if required

# TODO instead of doing this use the slurm redirect options
./ant_colony &> ant_colony.txt