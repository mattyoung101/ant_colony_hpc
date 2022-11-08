These are the final results for OpenMP. They were run using #SBATCH --constraint=R640 which means using these nodes:

smp-7-[0-2]          rack-0,28CPUs,R640 
smp-7-[3-4]          rack-0,36CPUs,R640 

see here for more info: https://stackoverflow.com/questions/37480603/slurm-how-to-run-30-jobs-on-particular-nodes-only

OMP32_goodcpu_antsparallel uses 32 cores but parallelises the ant update loop (inner loop) rather than
the colony update loop (outer loop)