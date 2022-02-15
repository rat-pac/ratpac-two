###########################################################
# CMakeLists for core module

add_library(core OBJECT
        src/GLG4HitPhoton.cc
        src/GLG4HitPMT.cc
        src/GLG4SteppingAction.cc
        src/PythonProc.cc
        src/ProcBlock.cc
        src/RunManager.cc
        src/CountProc.cc
        src/PruneProc.cc
        src/Gsim.cc
        src/ConstructUserProc.cc
        src/GLG4VisManager.cc
        src/GLG4VEventAction.cc
        src/StackingAction.cc
        src/Producer.cc
        src/Trajectory.cc
        src/Processor.cc
        src/SignalHandler.cc
        src/TrackInfo.cc
        src/Log.cc
        src/GLG4HitPMTCollection.cc)

# Set our include directories
target_include_directories(core PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
