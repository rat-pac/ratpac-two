############################################################
# CMakeLists for rat tool

add_executable(ntuple ntuple.cpp)
target_link_libraries(ntuple PUBLIC RATPAC)
add_dependencies(ntuple RATEvent)

install(TARGETS ntuple
        RUNTIME DESTINATION bin)
