###########################################################
# CMakeLists for stlplus module

add_library(stlplus OBJECT
        src/debug.cc
        src/dprintf.cc
        src/exceptions.cc
        src/file_system.cc
        src/fileio.cc
        src/multiio.cc
        src/os_fixes.cc
        src/persistent.cc
        src/string_utilities.cc
        src/stringio.cc
        src/textio.cc)

# Set our include directories
target_include_directories(stlplus SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(stlplus PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)
target_compile_options(stlplus PRIVATE -w)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
