import ROOT
from rat import RAT
import sys

'''
This example shows how to read a ROOT DS file
and look at the tracking information. The macro
must be run to store particle tracks using:
/tracking/storeTrajectory 1
and the output file line should be:
/rat/proc outroot
'''

ds = RAT.DSReader(sys.argv[1])

# Loop over the simulated events
for ev in range(ds.GetTotal()):
 
    r = ds.GetEvent(ev)

    mc = r.GetMC()
    tracks = mc.GetMCTrackCount()

    # Loop over all of the tracks
    for track in range(tracks): 

        mc_track = mc.GetMCTrack(track)
        steps = mc_track.GetMCTrackStepCount()

        # This will give an easy to read name
        name = mc_track.GetParticleName()
        pdg = mc_track.GetPDGCode()

        # Skip the photon tracks
        if(abs(pdg)==22): continue

        # Loop over each step along a track
        for step in range(steps):

            mc_step = mc_track.GetMCTrackStep(step)

            energy = mc_step.GetKE()
            energy_dep = mc_step.GetDepositedEnergy()

            # Where the step ended
            pos = mc_step.GetEndpoint()
            posx = pos[0] # same for posy,posz

            mom = mc_step.GetMomentum()
            momx = mom[0] # same for momy,momz 

            # Name of the detector volume the step is in
            vol = mc_step.GetVolume()

            # Start time of the step, relative 
            # to the start of the simulation
            time = mc_step.GetGlobalTime()

            # Physical process acting at endpoint
            process = mc_step.GetProcess()

