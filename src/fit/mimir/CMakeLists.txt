###########################################################
add_library(mimir OBJECT
  src/FitMimirProc.cc
  src/PMTTypeTimeResidualPDF.cc
  src/PMTTypeCosAlphaPDF.cc
  src/ParamSet.cc
  src/FitStep.cc
  src/RootOptimizer.cc
  src/FitSteps.cc
)

set_target_properties(mimir PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Set our include directories
target_include_directories(mimir SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(mimir PUBLIC
  $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
