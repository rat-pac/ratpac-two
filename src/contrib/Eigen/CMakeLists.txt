add_library(Eigen INTERFACE)

# Set our include directories
target_include_directories(Eigen SYSTEM INTERFACE 
	$<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/Eigen>)
target_include_directories(Eigen INTERFACE
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
