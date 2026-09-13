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
        src/textio.cc
)

# Set our include directories
target_include_directories(stlplus SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/stlplus>
        $<INSTALL_INTERFACE:include>
)
target_compile_options(stlplus PRIVATE -w)

target_compile_features(stlplus PRIVATE cxx_std_17)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
