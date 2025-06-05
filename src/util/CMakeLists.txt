###########################################################
# CMakeLists for util module

add_library(util OBJECT
        src/GLG4StringUtil.cc
        src/GaussianRatioPDF.cc
        src/PolygonOrientation.cc
        src/ReadFile.cc
        src/Sampling.cc
        src/SilenceRooFitBanner.cc
        src/SimpsIntegral.cc
        src/StringUtil.cc
        src/WaveformUtil.cc
        src/quadrature/Evaluateable.cc
        src/quadrature/Quadrature.cc
        src/quadrature/FixedTrapezoidalQuadrature.cc
        src/quadrature/AdaptiveSimpsonQuadrature.cc 
        )
# Set our include directories
target_include_directories(util SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(util PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})

#### Target: Shared library to make RooFit stop printing that stupid banner
add_library(SilenceRooFitBanner SHARED src/SilenceRooFitBanner.cc)

target_link_libraries(SilenceRooFitBanner PUBLIC ${ROOT_LIBRARIES})

set_target_properties( SilenceRooFitBanner
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

install(TARGETS SilenceRooFitBanner
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
