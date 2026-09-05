###########################################################
# CMakeLists for fit module
add_subdirectory(mimir)
add_library(fit_common OBJECT
  src/ClassifyChargeBalance.cc
  src/ClassifyTimesProc.cc
  src/FitPathProc.cc
  src/FitCentroidProc.cc
  src/FitTensorProc.cc
  src/FitQuadProc.cc
  src/FitDirectionCenterProc.cc
  src/MiniSim.cc
)
set_target_properties(fit_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
# FitTensorProc.{cc,hh} uses cppflow, which wraps TensorFlow.
target_link_libraries(fit_common PUBLIC ratpac_common cppflow Ratpac::TensorFlow)

# Create interface target to hold all fit objects
add_library(fit INTERFACE)
target_sources(fit INTERFACE
  $<TARGET_OBJECTS:mimir>
  $<TARGET_OBJECTS:fit_common>
)


# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
