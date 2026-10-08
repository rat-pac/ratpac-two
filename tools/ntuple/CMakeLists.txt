############################################################
# CMakeLists for rat tool

add_executable(ntuple ntuple.cpp)
target_link_libraries(ntuple PUBLIC RATPAC ratpac_common)
add_dependencies(ntuple RATPAC)

install(TARGETS ntuple
        RUNTIME DESTINATION bin
)
