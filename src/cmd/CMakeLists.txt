###########################################################
# CMakeLists for the cmd module

add_library(cmd OBJECT
        src/CoincidenceMessenger.cc
        src/DBMessenger.cc
        src/ESgenMessenger.cc
        src/FastNeutronMessenger.cc
        src/GLG4DebugMessenger.cc
        src/GLG4PrimaryGeneratorMessenger.cc
        src/GLG4VisMessenger.cc
        src/IsotopeMessenger.cc
        src/PhysicsListMessenger.cc
        src/ProcBlockManager.cc
        src/IBDgenMessenger.cc
        src/ReacIBDgenMessenger.cc
        src/SNgenMessenger.cc
        src/TrackingMessenger.cc)

# Set our include directories
target_include_directories(cmd PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
