###########################################################
# CMakeLists for quadrature module

add_library(quadrature OBJECT
    src/Evaluateable.cc
    src/Quadrature.cc
    src/FixedTrapezoidalQuadrature.cc
    src/AdaptiveSimpsonQuadrature.cc 
    )

# Set our include directories
target_include_directories(quadrature PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
