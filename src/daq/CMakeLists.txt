###########################################################
# CMakeLists for daq module

add_library(daq OBJECT
  src/Digitizer.cc
  src/LessSimpleDAQ2Proc.cc
  src/LessSimpleDAQProc.cc
  src/MiniCleanPMTCharge.cc
  src/NoiseProc.cc
  src/AfterPulseProc.cc
  src/PDFPMTCharge.cc
  src/PDFPMTTime.cc
  src/PMTPulse.cc
  src/PMTWaveformGenerator.cc
  src/PMTWaveform.cc
  src/SimpleDAQProc.cc
  src/SplitEVDAQProc.cc
  src/TrueDAQProc.cc
  src/WaveformAnalysis.cc
  )

# Set our include directories
target_include_directories(daq SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(daq PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
