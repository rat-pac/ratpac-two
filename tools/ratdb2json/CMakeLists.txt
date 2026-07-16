############################################################
# CMakeLists for rat tool

add_executable(ratdb2json ratdb2json.cpp)
target_link_libraries(ratdb2json PUBLIC RATPAC)
add_dependencies(ratdb2json RATPAC)

install(TARGETS ratdb2json
        RUNTIME DESTINATION bin)
