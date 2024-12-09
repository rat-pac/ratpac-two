###########################################################
# CMakeLists for gen module
set(gen_sources
    src/AmBeGen.cc
    src/AmBeSource.cc
    src/BetaFunction.cc
    src/CCCrossSec.cc
    src/CCgen.cc
    src/CfGen.cc
    src/CfSource.cc
    src/Coincidence_Gen.cc
    src/Decay0.cc
    src/DecayChain.cc
    src/DecayChain_Gen.cc
    src/ESCrossSec.cc
    src/ESgen.cc
    src/FermiFunction.cc
    src/GLG4DeferTrackProc.cc
    src/GLG4Gen.cc
    src/GLG4PosGen.cc
    src/GLG4PrimaryGeneratorAction.cc
    src/GLG4TimeGen.cc
    src/GLG4VertexGen.cc
    src/GdGen.cc
    src/HeGen.cc
    src/LiGen.cc
    src/NGen.cc
    src/Gen_LED.cc
    src/Gen_RandomTrigger.cc
    src/IBDgen.cc
    src/PosGen_FillShell.cc
    src/PosGen_Line.cc
    src/PosGen_Multipoint.cc
    src/PosGen_Radial.cc
    src/PosGen_RegexFill.cc
    src/PosGen_TriMeshSurface.cc
    src/ReacIBDgen.cc
    src/SNgen.cc
    src/VertexFile_Gen.cc
    src/RooTracker_Gen.cc
    src/VertexGen_CC.cc
    src/VertexGen_Decay0.cc
    src/VertexGen_ES.cc
    src/VertexGen_FastNeutron.cc
    src/VertexGen_IBD.cc
    src/VertexGen_Isotope.cc
    src/VertexGen_PhotonBomb.cc
    src/VertexGen_ReacIBD.cc
    src/VertexGen_SN.cc
    src/VertexGen_Spectrum.cc
    src/VertexGen_WIMP.cc)

if(${CRY_Enabled})
  set(gen_sources ${gen_sources} src/VertexGen_CRY.cc)
endif()

add_library(gen OBJECT ${gen_sources})

# Set our include directories
target_include_directories(gen SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(gen PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
