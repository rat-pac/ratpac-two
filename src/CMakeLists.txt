# Go through each module
add_subdirectory(cmd)
add_subdirectory(core)
add_subdirectory(daq)
add_subdirectory(db)
add_subdirectory(ds)
add_subdirectory(fit)
add_subdirectory(gen)
add_subdirectory(geo)
add_subdirectory(io)
add_subdirectory(physics)
add_subdirectory(ratbase)
add_subdirectory(util)
add_subdirectory(external)

# RATPAC_MODULE_INCLUDE_DIRS GLOBAL property via rat_register_include_dir()
get_property(RATPAC_MODULE_INCLUDE_DIRS GLOBAL PROPERTY RATPAC_MODULE_INCLUDE_DIRS)

###########################################################
# Create Root Dictionary
#
include_directories(${RATPAC_MODULE_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stlplus/include/stlplus
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stlplus/include
)
get_property(RATDICT_HEADERS GLOBAL PROPERTY RATDICT_HEADERS)
rat_generate_linkdef(RATDICT_LINKDEF ${CMAKE_CURRENT_BINARY_DIR}/RATDictLinkDef.hh)
root_generate_dictionary(G__RATDict
        ${RATDICT_HEADERS}
        LINKDEF ${RATDICT_LINKDEF}
        DEPENDENCIES ratpac_common Ratpac::Geant4
        OPTIONS -Wno-exceptions
)
add_library(RATDict OBJECT G__RATDict.cxx)
# G__RATDict.cxx #includes every collected RAT header, so it needs the same
# usage requirements those headers need to compile 
target_link_libraries(RATDict PUBLIC ratpac_common)

# Export the dictionary support files to the install tree.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
              ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
        DESTINATION lib
)


###########################################################
# Create our libraries and executables

set(RATPAC_MODULES
  cmd core daq db ds gen geo io fit physics ratbase stlplus cppflow util RATDict
)

# Create the shared RATPAC target for downstream experiments
# Add third party dependencies as PUBLIC and include all of the ratpac 
# code as PRIVATE  
add_library(RATPAC SHARED)
target_link_libraries(RATPAC
  PUBLIC Ratpac::ROOT Ratpac::Geant4 Ratpac::CRY Ratpac::TensorFlow Ratpac::NLopt
  PRIVATE ${RATPAC_MODULES}
)
# Give downstream experiments the merged include directory
target_include_directories(RATPAC PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories(RATPAC SYSTEM PUBLIC $<INSTALL_INTERFACE:include/stlplus>)

### library for ROOT
# TODO: this is a byte-by-byte identical copy to RATPAC.
# To make this a lighter weight library to read ratpac data some of the ratpac source 
#will need to be changed. 
add_library(RATEvent SHARED)
target_include_directories(RATEvent PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories(RATEvent SYSTEM PUBLIC $<INSTALL_INTERFACE:include/stlplus>)
target_link_libraries(RATEvent
  PUBLIC Ratpac::ROOT Ratpac::Geant4 Ratpac::CRY Ratpac::TensorFlow Ratpac::NLopt
  PRIVATE ${RATPAC_MODULES}
)

# This copies the root dict files to <build>/lib
add_custom_command(TARGET RATPAC POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
       ${PROJECT_BINARY_DIR}/lib
)

# This copies the root dict files to <build>/bin
add_custom_command(TARGET RATPAC POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
       ${PROJECT_BINARY_DIR}/bin
)

set_target_properties(RATPAC RATEvent
    PROPERTIES
       ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
)

# Prepare RATPAC and RATEvent for installation
install(TARGETS RATPAC RATEvent
        EXPORT RatpacTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
