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
        src/PhysicsList.cc
        src/G4CerenkovProcess.cc
        src/ThinnableG4Cerenkov.cc
        src/quenching/EnergyLossFunction.cc
        src/quenching/AppliedQuenchingModel.cc
        src/quenching/BirksLaw.cc
        src/quenching/QuenchingCalculator.cc
        src/quenching/NaiveQuenchingCalculator.cc
        src/quenching/IntegratedQuenchingCalculator.cc
        )

target_link_libraries(physics PUBLIC ratpac_common stlplus)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
