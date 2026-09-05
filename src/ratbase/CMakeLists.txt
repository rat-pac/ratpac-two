###########################################################
# CMakeLists for core module

add_library(ratbase OBJECT
  src/Rat.cc
  src/AnyParse.cc
)

target_link_libraries(ratbase PUBLIC ratpac_common)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
