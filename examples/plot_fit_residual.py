# Plot position fit residuals
# Usage: python plot_fit_residual.py <ntuple_filename> <fitter_1> ... <fitter_n>
import uproot
import numpy as np
import awkward as ak
import matplotlib.pyplot as plt
import pandas as pd
import sys

def main():

    fname = sys.argv[1]
    fitters = sys.argv[2:]
    print("Opening file", fname)
    print("Plotting fitters:", fitters)

    columns = ["mcx", "mcy", "mcz", "mcu", "mcv", "mcw"]
    for fitter in fitters:
        columns.extend([f"x_{fitter}", f"y_{fitter}", f"z_{fitter}", f"validposition_{fitter}"])
    with uproot.open(fname) as f:
        data = f['output'].arrays(columns, library='pd')
    
    mc_position = data[["mcx", "mcy", "mcz"]].to_numpy()
    mc_direction = data[["mcu", "mcv", "mcw"]].to_numpy()
    fitter_positions = {}
    fitter_validposition = {}
    for fitter in fitters:
        fitter_positions[fitter] = data[[f"x_{fitter}", f"y_{fitter}", f"z_{fitter}"]].to_numpy()
        fitter_validposition[fitter] = data[f"validposition_{fitter}"].to_numpy().astype(bool)


    residual_binning = np.linspace(-600, 600, 101)
    plt.figure()
    plt.rcParams['text.usetex'] = True
    plt.rcParams["font.family"] = "Times New Roman"
    for idx, qty in enumerate(['X', 'Y', 'Z']):
        plt.subplot(2, 2, idx + 1)
        for fitter in fitters:
            valid = fitter_validposition[fitter]
            residuals = fitter_positions[fitter][valid, idx] - mc_position[valid, idx]
            mean = np.mean(residuals[(residuals > residual_binning[0]) & (residuals < residual_binning[-1])])
            std = np.std(residuals[(residuals > residual_binning[0]) & (residuals < residual_binning[-1])])
            label = f"\\textbf{{{fitter}}}: eff={np.sum(valid)/len(valid):.2f}\n$\\mu$={mean:.2f} mm $\\sigma$={std:.2f} mm"
            plt.hist(residuals, bins=residual_binning, label=label, histtype='step')
        plt.xlabel(f"{qty} Residual (reco - truth) [mm]")
        plt.ylabel("Counts")
        plt.xlim(residual_binning[0], residual_binning[-1])
        plt.legend(loc="upper right")

    # Plot drive
    plt.subplot(2, 2, 4)
    drive_binning = np.linspace(-200, 400, 101)
    for fitter in fitters:
        valid = fitter_validposition[fitter]
        residual = fitter_positions[fitter][valid] - mc_position[valid]
        drive = np.einsum('ij,ij->i', residual, mc_direction[valid])
        mean = np.mean(drive[(drive > drive_binning[0]) & (drive < drive_binning[-1])])
        std = np.std(drive[(drive > drive_binning[0]) & (drive < drive_binning[-1])])
        label = f"\\textbf{{{fitter}}}: eff={np.sum(valid)/len(valid):.2f}\n$\\mu$={mean:.2f} mm $\\sigma$={std:.2f} mm"
        plt.hist(drive, bins=drive_binning, label=label, histtype='step')
        plt.xlabel(r"Drive $(\vec{x}_{reco} - \vec{x}_{truth})\cdot\hat{d}_{truth}$ [mm]")
        plt.ylabel("Counts")
        plt.xlim(drive_binning[0], drive_binning[-1])
        plt.legend(loc="upper right")
    plt.show()

if __name__ == "__main__":
    main()
