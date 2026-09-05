# Finds every third-party dependency RATPAC needs and wraps it in a
# target-based usage requirement 

###########################################################
# ROOT
#
# RATPAC_ROOT_FIND_COMPONENTS and RATPAC_ROOT_COMPONENTS are substituted into
# config/RatpacConfig.cmake.in (via @VAR@ in configure_package_config_file)
# so a downstream find_package(Ratpac) re-finds ROOT with the exact same
# components this project was actually built against
set(RATPAC_ROOT_FIND_COMPONENTS Minuit2 ROOTTPython MathMore)
find_package(ROOT CONFIG REQUIRED COMPONENTS ${RATPAC_ROOT_FIND_COMPONENTS})
include(${ROOT_USE_FILE})

# Aggregate all of the components
set(RATPAC_ROOT_COMPONENTS
    Core Imt RIO Net Hist Graf Graf3d Gpad ROOTDataFrame Tree TreePlayer Rint
    Postscript Matrix Physics MathCore Thread MultiProc ROOTVecOps
    Minuit2 ROOTTPython MathMore
)
add_library(Ratpac::ROOT INTERFACE IMPORTED)
foreach(_comp IN LISTS RATPAC_ROOT_COMPONENTS)
  if(TARGET ROOT::${_comp})
    target_link_libraries(Ratpac::ROOT INTERFACE ROOT::${_comp})
  endif()
endforeach()

###########################################################
# Geant4
#
set(RATPAC_GEANT4_VERSION 11.4 CACHE STRING "Geant4 version for find_package to locate")
find_package(Geant4 ${RATPAC_GEANT4_VERSION} REQUIRED OPTIONAL_COMPONENTS vis_all ui_all)
include(${Geant4_USE_FILE})

# Geant4 doesn't have proper cmake targets so have to construct them using legacy variables
add_library(Ratpac::Geant4 INTERFACE IMPORTED)
target_link_libraries(Ratpac::Geant4 INTERFACE ${Geant4_LIBRARIES})
target_include_directories(Ratpac::Geant4 SYSTEM INTERFACE
        ${Geant4_INCLUDE_DIRS} 
        ${CLHEP_INCLUDE_DIRS} 
        ${XercesC_INCLUDE_DIRS}
)
target_compile_definitions(Ratpac::Geant4 INTERFACE ${Geant4_DEFINITIONS})

###########################################################
# Threads
find_package(Threads REQUIRED)

###########################################################
# CURL
find_package(CURL REQUIRED)

###########################################################
# FFTW3
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED IMPORTED_TARGET fftw3)
if (NOT TARGET FFTW3::fftw3)
  # Add an alias for pkg-config 
  add_library(FFTW3::fftw3 ALIAS PkgConfig::FFTW3)
endif()

###########################################################
# NLopt
#
set(NLOPT_Enabled 0)
find_package(NLopt CONFIG QUIET)
if(NLopt_FOUND)
    message(STATUS "Compiling with NLOPT")
    set(NLOPT_Enabled 1)
else()
    message(STATUS "NLOPT Not Found")
endif()

add_library(Ratpac::NLopt INTERFACE IMPORTED)
if(NLOPT_Enabled)
  target_link_libraries(Ratpac::NLopt INTERFACE NLopt::nlopt)
endif()

###########################################################
# TensorFlow
#
# Tensorflow doesn't ship with a cmake config file so use pkg-config
set(TENSORFLOW_Enabled 0)
pkg_check_modules(TENSORFLOW QUIET IMPORTED_TARGET tensorflow)
if(TENSORFLOW_FOUND)
  message(STATUS "Compiling with Tensorflow")
  set(TENSORFLOW_Enabled 1)
else()
  message(STATUS "Tensorflow Not Found")
endif()

add_library(Ratpac::TensorFlow INTERFACE IMPORTED)
if(TENSORFLOW_Enabled)
  target_link_libraries(Ratpac::TensorFlow INTERFACE PkgConfig::TENSORFLOW)
endif()

###########################################################
# CRY
#
# Optional, enabled only via the $CRYLIB environment variable.
# CRY ships no CMake config package and isn't discoverable via 
# find_library(), so its location can only come from the user 
# pointing at it directly. 
#
set(CRY_Enabled 0)
if(DEFINED ENV{CRYLIB})
  message(STATUS "Compiling with CRY enabled")
  set(CRY_Enabled 1)
  set(CRYLIBDIR $ENV{CRYLIB})
  set(CRYINCLUDE $ENV{CRYINCLUDE})
  set(CRYDATA $ENV{CRYDATA})
  set(CRY_LIBRARIES CRY)
endif()

add_library(Ratpac::CRY INTERFACE IMPORTED)
if(CRY_Enabled)
  target_link_libraries(Ratpac::CRY INTERFACE ${CRY_LIBRARIES})
  target_include_directories(Ratpac::CRY SYSTEM INTERFACE ${CRYINCLUDE})
  target_link_directories(Ratpac::CRY INTERFACE ${CRYLIBDIR})
endif()

###########################################################
# ratpac_common
#
# Baseline usage requirements every RAT module needs: C++17, the collected
# header tree, Geant4, and ROOT.
#
add_library(ratpac_common INTERFACE)
target_compile_features(ratpac_common INTERFACE cxx_std_17)
target_link_libraries(ratpac_common INTERFACE Ratpac::Geant4 Ratpac::ROOT)
target_include_directories(ratpac_common INTERFACE $<INSTALL_INTERFACE:include>)

# Include stlplus headers
target_include_directories(ratpac_common SYSTEM INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/external/stlplus/include/stlplus>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/external/stlplus/include>
)
