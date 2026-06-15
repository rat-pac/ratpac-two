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

# Contribute this module's headers and LinkDef fragment to the RAT ROOT
# dictionary (RATDict).
rat_dictionary_linkdef(RAT/db/LinkDef.hh)
rat_dictionary_headers(
        RAT/DB.hh
        RAT/DBLink.hh
        RAT/DBTextLoader.hh
        RAT/DBJsonLoader.hh
        RAT/HTTPDownloader.hh)
