#!/bin/bash -l
#
#SBATCH --job-name=ant_colony
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --mem=32GB
#SBATCH --time=0-480:00
#SBATCH -e log_%j.err
#SBATCH -o log_%j.out
#SBATCH --mail-user=SodiumSandwich@outlook.com
#SBATCH --mail-type=ALL
# force running on a particular cluster (the Xeon Gold ones) for reproducibility
#SBATCH --constraint=R640

# Slurm job for the CPU (OpenMP/serial) version of the ant colony

# https://research.smp.uq.edu.au/computing/index.php?n=Getafix.HardwareConfiguration
# we should have up to about 192GB of RAM

module load gnu

# write out number of threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

date
# make sure we use /usr/bin/time (GNU time) which supports showing max memory, etc.
/usr/bin/time -v ./cmake-build-release-getafix/ant_colony antconfig_megamap.ini
