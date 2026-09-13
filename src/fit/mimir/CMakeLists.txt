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

target_include_directories(mimir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
