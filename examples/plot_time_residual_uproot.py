from awkward.operations import ak_broadcast_arrays
import numpy as np
import matplotlib.pyplot as plt
import uproot
import awkward as ak
import pandas as pd
import sys

# This script computes the time residual of simulated events based on ntuple information.
# Requirements to run this are: uproot, awkward, pandas, matplotlib, numpy. ROOT is not required. 
# This is meant to be a very pythonic and data-sciency example for how to handle ntuple files. 
# Usage: name-of-script.py input_file.ntuple.root
if __name__ == "__main__":
    ntuple_fname = sys.argv[1]
    with uproot.open(ntuple_fname) as rootfile:
        output_tree = rootfile['output']
        meta_tree = rootfile['meta']
        print("="*80)
        print("Meta Tree:")
        print("="*80)
        # Prints out every branch in the ROOT tree. Similar to Tree->Print()
        meta_tree.show()
        print("="*80)
        print("Output Tree:")
        print("="*80)
        output_tree.show()

        # The meta tree is only written once, so all the PMT information are storedin a vector. Hence the 0 index at the very end.
        pmt_info = meta_tree.arrays(["pmtX", "pmtY", "pmtZ", "pmtU", 'pmtV', 'pmtW'])[0]
        pmt_info = ak.to_dataframe(pmt_info)
        print("="*80)
        print("PMT Positions and Directions:")
        print("="*80)
        print(pmt_info)
    
        vertices = output_tree.arrays(["mcx", "mcy", "mcz", "mcu", "mcv", "mcw"])
        hit_branches = ["mcPEPMTID", "mcPEHitTime"]
        hits = {}
        for bname in hit_branches:
            hits[bname] = output_tree[bname].array()
        # Broadcast the event-level information to be the same shape as teh hits. This way each hit has an associated
        # Event vertex information, making it easier to handle hit information without worying about maintaining
        # event indexing.
        for vertex_branch in vertices.fields:
            hits[vertex_branch] = ak.broadcast_arrays(vertices[vertex_branch], hits[hit_branches[0]])[0]

        # Reduce all awkward arrays to be a flat 1D array. This removes event-level indexing, but recall that
        # we have already put all event level information to be the same shape as the hit information so 
        # we can still associate each hit to the MC information associated with them.
        for k in hits:
            hits[k] = ak.flatten(hits[k])
        hits = pd.DataFrame(hits)
        # Merge the PMT Info dataframe with the hits one, effectively associating each hit with their respective
        # PMT information.
        hits = pd.merge(hits, pmt_info, left_on='mcPEPMTID', right_index=True)

        # Compute time residuals
        # Assuming water here (with a refractive index of 1.33). Speed of light is approximately 300 mm / ns 
        group_velocity = 300 / 1.33
        position_diff = hits[['mcx', 'mcy', 'mcz']].to_numpy() - hits[['pmtX', 'pmtY', 'pmtZ']].to_numpy()
        time_of_flight = np.linalg.norm(position_diff, axis=1) / group_velocity
        hits['tresid'] = hits['mcPEHitTime'].to_numpy() - time_of_flight
        plt.hist(hits['tresid'], bins=np.linspace(-10, 25, 140), histtype='step', color='k')
        plt.xlabel("Time Residual [ns]")
        plt.ylabel("Hits")
        plt.semilogy()
        plt.show()
