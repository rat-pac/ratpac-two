###########################################################
add_library(mimir OBJECT
  src/FitMimirProc.cc
  src/PMTTypeTimeResidualPDF.cc
  src/PMTTypeCosAlphaPDF.cc
  src/ParamSet.cc
  src/FitStep.cc
  src/RootOptimizer.cc
  src/FitSteps.cc
  src/NLOPTOptimizer.cc
)

set_target_properties(mimir PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(mimir PUBLIC ratpac_common Ratpac::NLopt)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
