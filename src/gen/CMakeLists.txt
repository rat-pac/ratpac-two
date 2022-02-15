###########################################################
# CMakeLists for gen module

add_library(gen OBJECT
        src/AmBeGen.cc
        src/AmBeSource.cc
        src/BetaFunction.cc
        src/CfGen.cc
        src/CfSource.cc
        src/Coincidence_Gen.cc
        src/DecayChain.cc
        src/DecayChain_Gen.cc
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
        src/VertexGen_ES.cc
        src/VertexGen_FastNeutron.cc
        src/VertexGen_IBD.cc
        src/VertexGen_Isotope.cc
        src/VertexGen_PhotonBomb.cc
        src/VertexGen_ReacIBD.cc
        src/VertexGen_SN.cc
        src/VertexGen_Spectrum.cc
        src/VertexGen_WIMP.cc)

# Set our include directories
target_include_directories(gen PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
