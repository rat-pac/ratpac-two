add_library(cppflow INTERFACE)

# Set our include directories
target_include_directories(cppflow SYSTEM INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
