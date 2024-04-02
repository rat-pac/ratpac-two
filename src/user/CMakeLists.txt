###########################################################
# CMakeLists for user module

add_library(user OBJECT
        src/UserAnalProc.cc)

# Set our include directories
target_include_directories(user SYSTEM PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(user PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
