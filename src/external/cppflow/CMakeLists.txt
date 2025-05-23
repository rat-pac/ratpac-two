add_library(cppflow INTERFACE)

# Set our include directories
target_include_directories(cppflow SYSTEM INTERFACE 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/cppflow>)
target_include_directories(cppflow INTERFACE
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
