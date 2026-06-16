###########################################################
# CMakeLists for ds module

add_library(ds OBJECT
        src/Root.cc
        src/MCPMT.cc
        src/RunStore.cc)
# Set our include directories
target_include_directories(ds SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(ds PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})

#################################################
# Contribute this module's headers and LinkDef fragment to the RAT ROOT
# dictionary (RATDict). The dictionary is generated once in src/CMakeLists.txt
# from the headers and fragments gathered across all subdirectories.
rat_dictionary_linkdef(RAT/DS/LinkDef.hh)
rat_dictionary_headers(
        RAT/DS/Root.hh
        RAT/DS/RootFactory.hh
        RAT/DS/Classifier.hh
        RAT/DS/Digit.hh
        RAT/DS/EV.hh
        RAT/DS/FitResult.hh
        RAT/DS/LAPPD.hh
        RAT/DS/LAPPDHit.hh
        RAT/DS/MC.hh
        RAT/DS/MCParticle.hh
        RAT/DS/MCPhoton.hh
        RAT/DS/MCPMT.hh
        RAT/DS/MCNestedTube.hh
        RAT/DS/MCNestedTubeHit.hh
        RAT/DS/MCSummary.hh
        RAT/DS/PMT.hh
        RAT/DS/RunStore.hh
        RAT/DS/Run.hh
        RAT/DS/PMTInfo.hh
        RAT/DS/NestedTubeInfo.hh
        RAT/DS/ChannelStatus.hh
        RAT/DS/MCTrack.hh
        RAT/DS/MCTrackStep.hh
        RAT/DS/Calib.hh
        RAT/DS/WaveformAnalysisResult.hh)
