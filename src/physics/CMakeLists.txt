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
        src/ThinnableG4Cerenkov.cc
        src/quenching/EnergyLossFunction.cc
        src/quenching/AppliedQuenchingModel.cc
        src/quenching/BirksLaw.cc
        src/quenching/QuenchingCalculator.cc
        src/quenching/NaiveQuenchingCalculator.cc
        src/quenching/IntegratedQuenchingCalculator.cc
        src/ANNRI_Gd/ANNRIGd_156GdContinuumModelV2.cc
        src/ANNRI_Gd/ANNRIGd_156GdDiscreteModel.cc
        src/ANNRI_Gd/ANNRIGd_158GdContinuumModelV2.cc
        src/ANNRI_Gd/ANNRIGd_158GdDiscreteModel.cc
        src/ANNRI_Gd/ANNRIGd_Auxiliary.cc
        src/ANNRI_Gd/ANNRIGd_DummyModel.cc
        src/ANNRI_Gd/ANNRIGd_GdNCaptureGammaGenerator.cc
        src/ANNRI_Gd/ANNRIGd_GeneratorConfigurator.cc
        src/ANNRI_Gd/ANNRIGd_Model.cc
        src/ANNRI_Gd/ANNRIGd_ModelType.cc
        src/ANNRI_Gd/ANNRIGd_OutputConverter.cc
        src/ANNRI_Gd/ANNRIGd_ReactionProduct.cc
        src/ANNRI_Gd/GdNeutronHPCapture.cc
        src/ANNRI_Gd/GdNeutronHPCaptureFS.cc
        src/MyG4HadronPhysicsShielding.cc
        src/MyG4HadronPhysicsShieldingLEND.cc
        src/MyG4NeutronPHPBuilder.cc
        src/MyShielding.cc
        )

# Set our include directories
target_include_directories(physics SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(physics PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
