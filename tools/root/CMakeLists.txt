############################################################
# CMakeLists for root tool

add_executable(root root.cc)
target_link_libraries(root PUBLIC ${ROOT_LIBRARIES} Threads::Threads)
# Add dependency because we're not linking, but the root.cc adds it dynamically
add_dependencies(root RATEvent)

file(COPY rootinit.C DESTINATION ${PROJECT_BINARY_DIR}/src)

# Make sure we don't overwrite original root
if(EXISTS "${CMAKE_INSTALL_PREFIX}/bin/root" AND RATPAC_ROOT)
       message(WARNING "${CMAKE_INSTALL_PREFIX}/bin/root already exists. If you wish to install to this directory you must remove or rename this file.")
endif()

install(TARGETS root
        RUNTIME DESTINATION bin)
install(FILES rootinit.C
       DESTINATION src)