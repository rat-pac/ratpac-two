###########################################################
add_library(mimir STATIC
  src/FitMimirProc.cc
  src/PMTTypeTimeResidualPDF.cc
  src/ParamSet.cc
  src/FitStep.cc
  src/RootOptimizer.cc
)

# Set our include directories
target_include_directories(mimir SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(mimir PUBLIC
  $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
