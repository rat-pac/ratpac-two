###########################################################
# CMakeLists for gen module

add_library(du OBJECT
    src/WaveformAnalysis.cc
  )

# Set our include directories
target_include_directories(du PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
