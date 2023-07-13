###########################################################
# CMakeLists for quenching module

add_library(quenching OBJECT
    src/EnergyLossFunction.cc
    src/AppliedQuenchingModel.cc
    src/BirksLaw.cc
    src/QuenchingCalculator.cc
    src/NaiveQuenchingCalculator.cc
    src/IntegratedQuenchingCalculator.cc
    )

# Set our include directories
target_include_directories(quenching PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
