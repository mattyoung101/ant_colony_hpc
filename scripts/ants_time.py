import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import tarfile

FILES = {
    # Impact of ant count on performance graph (each using same CPU on getafix, smp-7-whatever)
    # "Release Serial": "../results/results_omp_final/SERIAL_goodcpu_ants_02-11-2022_00-08-20.tar",
    # "Release OpenMP (16 threads)": "../results/results_omp_final/OMP16_goodcpu_ants_02-11-2022_00-10-43.tar",
    # "Release OpenMP (32 threads)": "../results/results_omp_final/OMP32_goodcpu_ants_02-11-2022_00-10-11.tar",

    # Comparing paralleling the ant update loop vs. parallelising the colony update loop
    # "Parallel colony update": "../results/results_omp_final/OMP32_goodcpu_ants_02-11-2022_00-10-11.tar",
    # "Parallel ant update": "../results/results_omp_final/OMP32_goodcpu_antsparallel_ants_05-11-2022_23-47-56.tar"

    # this one is for my  PC
    # "Release OpenMP (32 threads) (Ryzen 9 5950X)": "../results/results_omp_final/OMP32_Ryzen_ants_01-11-2022_06-20-14.tar"
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
    ax.legend()
    ax.grid()
    plt.show()
