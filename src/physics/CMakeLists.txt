###########################################################
# CMakeLists for physics module

add_library(physics OBJECT
        src/BNLOpWLS.cc
        src/BNLOpWLSBuilder.cc
        src/BNLOpWLSData.cc
        src/Dicebox158Gd.cc
        src/G4OpWLSBuilder.cc
        src/GLG4OpAttenuation.cc
        src/GLG4PMTOpticalModel.cc
        src/GLG4Scint.cc
        src/OpRayleigh.cc
        src/PhotonThinning.cc
        src/PhysicsList.cc)

# Set our include directories
target_include_directories(physics PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
