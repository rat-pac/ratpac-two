############################################################
# CMakeLists for rat tool

add_executable(rat rat.cc)
target_link_libraries(rat PUBLIC RATPAC)

install(TARGETS rat
        RUNTIME DESTINATION bin)