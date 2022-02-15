###########################################################
# CMakeLists for daq module

add_library(daq OBJECT
        src/LessSimpleDAQProc.cc
        src/SplitEVDAQProc.cc
        src/MiniCleanPMTCharge.cc
        src/PDFPMTCharge.cc
        src/LessSimpleDAQ2Proc.cc
        src/PDFPMTTime.cc
        src/NoiseProc.cc
        src/SimpleDAQProc.cc)

# Set our include directories
target_include_directories(daq PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCDIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCDIR})
