###########################################################
# CMakeLists for io module

add_library(io OBJECT
        src/DSReader.cc
        src/DSWriter.cc
        src/TrackCursor.cc
        src/TrackNav.cc
        src/InNetProducer.cc
        src/InROOTProducer.cc
        src/OutNetProc.cc
        src/OutNtupleProc.cc
        src/OutROOTProc.cc
)

target_link_libraries(io PUBLIC ratpac_common stlplus)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)

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
        RAT/ObjDbl.hh
)
