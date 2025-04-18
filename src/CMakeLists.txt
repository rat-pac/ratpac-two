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
# ds has to come last to avoid race condition with ratdict 
add_subdirectory(ds)
add_subdirectory(ratbase)



###########################################################
# Create our libraries and executables

# RAT shared library
add_library(RATPAC SHARED)
target_link_libraries(RATPAC PUBLIC
  ${ROOT_LIBRARIES} ${Geant4_LIBRARIES} ${CRY_LIBRARIES} ${TENSORFLOW_LIBRARIES}
  cmd core daq db ds fit gen geo io physics ratbase stlplus cppflow util RATDict)
target_include_directories(RATPAC PUBLIC ${CLHEP_INCLUDE_DIRS})
message("Including ${CLHEP_INCLUDE_DIRS}")


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
add_custom_command(TARGET RATEvent POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/ds/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/ds/libRATDict.rootmap
       ${PROJECT_BINARY_DIR}/lib)

# This copies the root dict files to <build>/lib
add_custom_command(TARGET RATEvent POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_BINARY_DIR}/ds/libRATDict_rdict.pcm
       ${CMAKE_CURRENT_BINARY_DIR}/ds/libRATDict.rootmap
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
