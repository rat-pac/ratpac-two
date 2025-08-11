###########################################################
# CMakeLists for fit module

add_library(fit OBJECT
  src/ClassifyChargeBalance.cc
  src/FitPathProc.cc
  src/FitCentroidProc.cc
  src/FitTensorProc.cc
  src/FitQuadProc.cc
  src/FitDirectionCenterProc.cc
  src/MiniSim.cc
  src/bonsai/ariadne.cc 
  src/bonsai/azimuth_ks.cc 
  src/bonsai/timefit.cc 
  src/bonsai/distpmt.cc 
  src/bonsai/likelihood.cc 
  src/bonsai/goodness.cc 
  src/bonsai/fit_param.cc 
  src/bonsai/fourhitgrid.cc 
  src/bonsai/pmt_geometry.cc 
  src/bonsai/hitsel.cc 
  src/bonsai/hits.cc 
  src/bonsai/bonsaifit.cc 
  src/bonsai/plato.cc 
  src/bonsai/binfile.cc 
  src/bonsai/centroid.cc
  src/bonsai/searchgrid.cc
  src/FitBonsaiProc.cc 
)



# Set our include directories
target_include_directories(fit SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(fit PUBLIC
  $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
