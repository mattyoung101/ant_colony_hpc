import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import tarfile

FILES = {
    "Release Serial": "../results/SERIAL_ants_31-10-2022_05-02-21.tar",
    "Release OpenMP (16 threads)": "../results/OMP_ants_31-10-2022_05-01-22.tar",
    # TODO release OpenMP 32 threads (currently running)
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
    ax.set_title("Impact of ant count on performance (Getafix, megamap)")
    ax.legend()
    ax.grid()
    plt.show()
