###########################################################
# CMakeLists for the cmd module

add_library(cmd OBJECT
        src/CCCrossSecMessenger.cc
        src/CoincidenceMessenger.cc
        src/DBMessenger.cc
        src/ESCrossSecMessenger.cc
        src/FastNeutronMessenger.cc
        src/G4UIcmdWithOptionalStringAndADoubleAndUnit.cc
        src/GLG4DebugMessenger.cc
        src/GLG4PrimaryGeneratorMessenger.cc
        src/GLG4VisMessenger.cc
        src/IsotopeMessenger.cc
        src/PhysicsListMessenger.cc
        src/ProcBlockManager.cc
        src/IBDgenMessenger.cc
        src/RatMessenger.cc
        src/ReacIBDgenMessenger.cc
        src/SNgenMessenger.cc
        src/TrackingMessenger.cc
)

target_link_libraries(cmd PUBLIC ratpac_common cppflow Ratpac::TensorFlow)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
