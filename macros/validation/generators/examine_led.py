import numpy as np
import matplotlib.pyplot as plt
import uproot
import pandas as pd
import awkward as ak
import numba

#|%%--%%| <1ySE3cRtJW|BHtCFCAIWM>

tfile = uproot.open("output.ntuple.root")
ttree = tfile['output']

photon_energy = ttree['trackKE'].array()
primary_photon = ak.flatten(photon_energy[:,:,0])

hbarc = 197.3269718 # MeV fm
convert_to_wavelength = lambda x: hbarc / x * 2 * np.pi / 1e6
plt.hist(convert_to_wavelength(primary_photon), bins=100)
plt.xlabel("Wavelength (nm)")
plt.show()

akw = ttree['trackTime'].array()
plt.hist(ak.flatten(akw[:,:,0]), bins=100)
plt.xlabel("Time (ns)")
plt.show()
