# CMakeLists.txt for the ratpac library code
# Author: Miles Lucas

###########################################################
# Go through each module and get their targets

add_subdirectory(cmd)
add_subdirectory(core)
add_subdirectory(daq)
add_subdirectory(db)
add_subdirectory(fit)
add_subdirectory(gen)
add_subdirectory(geo)
add_subdirectory(io)
add_subdirectory(physics)
add_subdirectory(external)
add_subdirectory(util)
add_subdirectory(ds)
add_subdirectory(ratbase)


###########################################################
# Create Root Dictionary
#
# Subdirectories register their headers and LinkDef fragments via
# rat_dictionary_headers()/rat_dictionary_linkdef(); we generate the single
# RATDict here, once every subdirectory has contributed. RATPAC_INCLUDE_DIR must
# be on the include path so rootcling can resolve the collected header names.
include_directories(${RATPAC_INCLUDE_DIR})
get_property(RATDICT_HEADERS GLOBAL PROPERTY RATDICT_HEADERS)
rat_generate_linkdef(RATDICT_LINKDEF ${CMAKE_CURRENT_BINARY_DIR}/RATDictLinkDef.hh)
root_generate_dictionary(G__RATDict
        ${RATDICT_HEADERS}
        LINKDEF ${RATDICT_LINKDEF}
        DEPENDENCIES core ds db io util
        OPTIONS -Wno-exceptions)
add_library(RATDict OBJECT G__RATDict.cxx)
target_include_directories(RATDict SYSTEM PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(RATDict PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Export the dictionary support files to the install tree.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
              ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
        DESTINATION lib)


###########################################################
# Create our libraries and executables

# RAT shared library
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so") # for macOS users, cmake produces dynamic library (.dylib) files instead of share objects (.so) - this just forces the latter
add_library(RATPAC SHARED)
target_link_libraries(RATPAC PUBLIC
  ${ROOT_LIBRARIES} ${Geant4_LIBRARIES} ${CRY_LIBRARIES} ${TENSORFLOW_LIBRARIES} ${NLOPT_LIBRARIES}
  cmd core daq db ds gen geo io fit physics ratbase stlplus cppflow util RATDict)
target_include_directories(RATPAC PUBLIC 
       ${CLHEP_INCLUDE_DIRS} 
       ${NLOPT_INCLUDE_DIRS} 
       ${TENSORFLOW_INCLUDE_DIRS}
       )
message("Including ${CLHEP_INCLUDE_DIRS} ${NLOPT_INCLUDE_DIRS} ${TENSORFLOW_INCLUDE_DIRS}")


### Target: library for ROOT
add_library(RATEvent SHARED
       core/src/Log.cc
       db/src/DB.cc
       db/src/DBJsonLoader.cc
       db/src/DBLink.cc
       db/src/DBTable.cc
       db/src/DBTextLoader.cc
       db/src/HTTPDownloader.cc
       db/src/json.cc
       ds/src/MCPMT.cc
       ds/src/Root.cc
       ds/src/RunStore.cc
       io/src/DSReader.cc
       io/src/DSWriter.cc
       io/src/TrackCursor.cc
       io/src/TrackNav.cc
       util/src/ReadFile.cc)
target_include_directories(RATEvent SYSTEM PUBLIC ${ROOT_INCLUDE_DIRS}/stlplus)
target_include_directories(RATEvent PUBLIC ${RATPAC_INCLUDE_DIR})
target_link_libraries(RATEvent PUBLIC 
       ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} RATDict stlplus)

# This copies the root dict files to <build>/lib
add_custom_command(TARGET RATPAC POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
       ${PROJECT_BINARY_DIR}/lib)

# This copies the root dict files to <build>/bin
add_custom_command(TARGET RATPAC POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/libRATDict.rootmap
       ${PROJECT_BINARY_DIR}/bin)

set_target_properties(RATPAC RATEvent 
    PROPERTIES
       ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

install(TARGETS RATPAC RATEvent
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

### Install the global headers
install(DIRECTORY ${RATPAC_INCLUDE_DIR} 
       DESTINATION ${CMAKE_INSTALL_PREFIX})
