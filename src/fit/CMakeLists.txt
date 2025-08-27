###########################################################
# CMakeLists for fit module
add_subdirectory(mimir)
add_library(fit_common OBJECT
  src/ClassifyChargeBalance.cc
  src/FitPathProc.cc
  src/FitCentroidProc.cc
  src/FitTensorProc.cc
  src/FitQuadProc.cc
  src/FitDirectionCenterProc.cc
  src/MiniSim.cc)
set_target_properties(fit_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Set our include directories
target_include_directories(fit_common SYSTEM PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(fit_common PUBLIC
  $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

# Create interface target to hold all fit objects
add_library(fit INTERFACE)
target_sources(fit INTERFACE
  $<TARGET_OBJECTS:mimir>
  $<TARGET_OBJECTS:fit_common>
)


# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
