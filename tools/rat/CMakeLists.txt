############################################################
# CMakeLists for rat tool

add_executable(rat rat.cpp)
# RATPAC links PRIVATEly against ratpac modules so still need ratpac_common
target_link_libraries(rat PUBLIC RATPAC ratpac_common)

install(TARGETS rat
        RUNTIME DESTINATION bin
)
