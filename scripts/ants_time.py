import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import tarfile

FILES = {
    # Impact of ant count on performance graph (each using same CPU on getafix, smp-7-whatever)
    "Release Serial": "../results/results_omp_final/SERIAL_goodcpu_ants_02-11-2022_00-08-20.tar",
    "Release OpenMP (16 threads)": "../results/results_omp_final/OMP16_goodcpu_ants_02-11-2022_00-10-43.tar",
    "Release OpenMP (24 threads)": "../results/results_omp_final/OMP24_goodcpu_ants_09-11-2022_15-54-37.tar",
    "Release OpenMP (32 threads)": "../results/results_omp_final/OMP32_goodcpu_ants_02-11-2022_00-10-11.tar",

    # Comparing paralleling the ant update loop vs. parallelising the colony update loop
    # "Parallel colony update": "../results/results_omp_final/OMP32_goodcpu_ants_02-11-2022_00-10-11.tar",
    # "Parallel ant update": "../results/results_omp_final/OMP32_goodcpu_antsparallel_ants_05-11-2022_23-47-56.tar"

    # this one is for my  PC
    # "Release Serial (Ryzen 9 5950X)": "../results/results_serial_final/SERIAL_ryzen_ants_13-11-2022_15-09-52.tar"

    # MPI
    # "Release Serial": "../results/results_mpi_final/MPI_serialcomparison_ants_ants_14-11-2022_01-53-04.tar",
    # "Release OpenMP (20 threads)": "../results/results_mpi_final/MPI_ompcomparison_20threads_ants_14-11-2022_02-02-35.tar",
    # "Release MPI (20 workers, 5 workers per node)": "../results/results_mpi_final/MPI_20_5_ants_ants_14-11-2022_01-51-00.tar",
    # "Release MPI (20 workers, 10 workers per node)": "../results/results_mpi_final/MPI_20_10_ants_ants_14-11-2022_01-51-40.tar",
}

if __name__ == "__main__":
    fig, ax = plt.subplots(figsize=(10, 8), dpi=80)

    for name, path in FILES.items():
        tar = tarfile.open(path)
        # turn duplicate rows into their averages: https://stackoverflow.com/a/70187952/5007892
        data = pd.read_csv(tar.extractfile("ants_vs_time.csv")).groupby(["NumAnts"]).mean()
        # TODO plot uncertainty as well (less samples means less sure)
        ax.plot(data, label=name)

    ax.set_ylabel("Average iteration time (milliseconds)")
    ax.set_xlabel("Number of ants")
    ax.set_title("Impact of ant count on performance (getafix, megamap)")
    # ax.set_title("Impact of ant count on performance (personal workstation, megamap)")
    # ax.set_title("Impact of ant count on performance (getafix, megamap_mpi)")
    ax.legend()
    ax.grid()
    plt.show()
