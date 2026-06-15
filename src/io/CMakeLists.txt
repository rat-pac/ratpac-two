###########################################################
# CMakeLists for io module

add_library(io OBJECT
        src/DSReader.cc
        src/DSWriter.cc
        src/InNetProducer.cc
        src/InROOTProducer.cc
        src/OutNetProc.cc
        src/OutNtupleProc.cc
        src/OutROOTProc.cc
        src/TrackCursor.cc
        src/TrackNav.cc)

# Set our include directories
target_include_directories(io SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(io PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})

# Contribute this module's headers and LinkDef fragment to the RAT ROOT
# dictionary (RATDict).
rat_dictionary_linkdef(RAT/io/LinkDef.hh)
rat_dictionary_headers(
        RAT/DSReader.hh
        RAT/DSWriter.hh
        RAT/TrackNav.hh
        RAT/TrackNode.hh
        RAT/TrackCursor.hh
        RAT/ObjInt.hh
        RAT/ObjDbl.hh)
