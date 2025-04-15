###########################################################
# CMakeLists for fit module

add_library(fit OBJECT
  src/ClassifyChargeBalance.cc
  src/FitPathProc.cc
  src/FitCentroidProc.cc
  src/FitTensorProc.cc
  src/FitQuadProc.cc
  src/FitDirectionCenterProc.cc
  src/MiniSim.cc)


# Set our include directories
target_include_directories(fit SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(fit PUBLIC
  $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
