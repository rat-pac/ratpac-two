###########################################################
# CMakeLists for util module

add_library(util OBJECT
        src/ReadFile.cc
        src/GLG4StringUtil.cc
        src/GaussianRatioPDF.cc
        src/PolygonOrientation.cc
        src/NNLS.cc
        src/NPEEstimator.cc
        src/Sampling.cc
        src/SilenceRooFitBanner.cc
        src/SimpsIntegral.cc
        src/StringUtil.cc
        src/WaveformUtil.cc
        src/quadrature/Evaluateable.cc
        src/quadrature/Quadrature.cc
        src/quadrature/FixedTrapezoidalQuadrature.cc
        src/quadrature/AdaptiveSimpsonQuadrature.cc 
        src/LightPathCalculator.cc
        src/GroupVelocityCalculator.cc
        src/TransitTimeCalculator.cc
        )
target_link_libraries(util PUBLIC ratpac_common FFTW3::fftw3)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)

# Contribute this module's headers and LinkDef fragment to the RAT ROOT
# dictionary (RATDict).
rat_dictionary_linkdef(RAT/util/LinkDef.hh)
rat_dictionary_headers(RAT/TransitTimeCalculator.hh)

#### Target: Shared library to make RooFit stop printing that stupid banner
add_library(SilenceRooFitBanner SHARED src/SilenceRooFitBanner.cc)

target_link_libraries(SilenceRooFitBanner PUBLIC Ratpac::ROOT)

set_target_properties( SilenceRooFitBanner
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
)

install(TARGETS SilenceRooFitBanner
  EXPORT RatpacTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
