###########################################################
# CMakeLists for db module

add_library(db OBJECT
        src/DB.cc
        src/DBJsonLoader.cc
        src/DBLink.cc
        src/DBTable.cc
        src/DBTextLoader.cc
        src/HTTPDownloader.cc
        src/json.cc
)
target_link_libraries(db PUBLIC ratpac_common stlplus CURL::libcurl)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)

# Contribute this module's headers and LinkDef fragment to the RAT ROOT
# dictionary (RATDict).
rat_dictionary_linkdef(RAT/db/LinkDef.hh)
rat_dictionary_headers(
        RAT/DB.hh
        RAT/DBLink.hh
        RAT/DBTextLoader.hh
        RAT/DBJsonLoader.hh
        RAT/HTTPDownloader.hh
)
