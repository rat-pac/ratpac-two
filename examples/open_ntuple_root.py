import ROOT
import sys

'''
An example for opening a RAT ntuple file.
'''

def open_ntuple(filename):

    # Open the ntuple file. Requires we ran the macro
    # with the outntuple processor.
    f = ROOT.TFile.Open(filename)

    # The output tree has entries for each event 
    outputTree = f.Get("output")

    # The meta tree has a single entry for each simulation
    metaTree = f.Get("meta")

    '''
    If we ran the macro with:
    rat/proc outntuple
    /rat/procset include_digitizerwaveforms 1
    then we will store the waveforms to this tree:
    '''
    # waveform = f.Get("waveforms")

    '''
    Should just be one entry in the meta branch
    For this example, lets get a map of pmtIDs to 
    the pmt positions
    '''
    for iMeta in range(metaTree.GetEntries()):

        metaTree.GetEntry(iMeta)

        # Some examples!
        # runID = metaTree.runID
        # runType = metaTree.runType
        # digitizerWindowSize = metaTree.digitizerWindowSize

        # Lets get the PMT information
        pmtId = list(metaTree.pmtId)
        pmtX = list(metaTree.pmtX)
        pmtY = list(metaTree.pmtY)
        pmtZ = list(metaTree.pmtZ)    

    '''
    Loop over the entries in the output branch.
    We give a bunch of example below, but please
    see the documentation for details about the 
    ntuple branches.
    '''
    for i in range(outputTree.GetEntries()):

        outputTree.GetEntry(i)

        # PDG for the first MC particle
        mcpdg = outputTree.mcpdg
        # Kinetic energy for the first MC particle
        mcke = outputTree.mcke
        # The number of MC particles 
        mcparticlecount = outputTree.mcparticlecount
        # The number of MC photoelectrons
        mcpecount = outputTree.mcpecount

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_mcparticles 1
        then we save information for all particles:
        This is turned off by default.
        '''
        # mcpdgs = list(outputTree.mcpdgs)
        # mckes = list(outputTree.mckes)

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_mchits 1
        then we save information for all true PMT hits
        This is turned off by default.
        '''
        #mcpmtid = list(outputTree.mcPMTID)
        #mcpmtnpe = list(outputTree.mcPMTNPE)

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_pmthits 1
        then we save information for all detector event PMT hits.
        This is turned on by default.
        '''
        pmtid = list(outputTree.hitPMTID)
        pmttime = list(outputTree.hitPMTTime)

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_digitzerhits 1
        then we save information from the waveform analysis
        This is turned on by default, but the arrays will be
        empty if you have not run the waveform prep processor:
        /rat/proc WaveformPrep
        '''
        digit_pmtid = list(outputTree.digitPMTID)
        digit_pmttime = list(outputTree.digitTime)

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_digitzerfits 1
        then we save information from the waveform fits
        This is turned on by default.
        A waveform analysis processor must have been run from
        the macro, we assume below that the lognormal fit was run:
        /rat/proc WaveformAnalysisLognormal
        '''
        #fit_pmtid_lognormal = list(outputTree.fit_pmtid_Lognormal)
        #fit_pmttime_lognormal = list(outputTree.fit_time_Lognormal)

        '''
        If we run our macro with:
        /rat/proc outntuple
        /rat/procset include_tracking 1
        then we save the full particle tracking information
        This is turned off by default. We must also have:
        /tracking/storeTrajectory 1
        turned on the macro.
        '''
        #trackPDG = list(outputTree.trackPDG)
        #trackStepPosX = list(list(outputTree.trackPosX))

        '''
        This gives an idea of how to loop through the track steps
        '''
        #for iTrack in range(len(trackPDG)):
        #    trackStepX = trackStepPosX[iTrack]
        #    for iStep in range(len(trackStepX)):
        #        stepX = trackStepX[iStep]
        #        print (stepX)
 
        '''
        If we ran a reconstruction algorithm, such as the quad
        fitter, then we can access reconstructed information using
        the fitter name. This requires the macro ran:
        /rat/proc quadfitter
        '''
        # fittedX = outputTree.x_quadfitter
        # fittedY = outputTree.y_quadfitter
        # fittedZ = outputTree.z_quadfitter


open_ntuple(sys.argv[1])

