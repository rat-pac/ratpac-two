###########################################################
# CMakeLists for core module

add_library(ratbase OBJECT
  src/Rat.cc
  src/AnyParse.cc)

# Set our include directories
target_include_directories(ratbase SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(ratbase PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
