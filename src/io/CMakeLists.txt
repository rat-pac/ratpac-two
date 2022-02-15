###########################################################
# CMakeLists for io module

add_library(io OBJECT
        src/DSReader.cc
        src/DSWriter.cc
        src/InNetProducer.cc
        src/InROOTProducer.cc
        src/OutNetProc.cc
        src/OutROOTProc.cc
        src/TrackCursor.cc
        src/TrackNav.cc)

# Set our include directories
target_include_directories(io PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
