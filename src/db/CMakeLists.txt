###########################################################
# CMakeLists for db module

add_library(db OBJECT
        src/DBTextLoader.cc
        src/HTTPDownloader.cc
        src/json.cc
        src/DBLink.cc
        src/DB.cc
        src/DBJsonLoader.cc
        src/DBTable.cc)

# Set our include directories
target_include_directories(db SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(db PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
