#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH --mem=12GB
#SBATCH --time=0-60:00
#SBATCH --mail-user=SodiumSandwich@outlook.com
#SBATCH --mail-type=ALL
#SBATCH -e ant_colony.err
#SBATCH -o ant_colony.out

module load gnu
module load openmpi3_eth/3.0.0
# CUDA if required (make sure to make it run on a GPU partition then)

date
echo ""
time ./ant_colony