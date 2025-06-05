###########################################################
# CMakeLists for daq module

add_library(daq OBJECT
  src/Digitizer.cc
  src/NoiseProc.cc
  src/AfterPulseProc.cc
  src/PDFPMTCharge.cc
  src/PDFPMTTime.cc
  src/PMTPulse.cc
  src/PMTWaveformGenerator.cc
  src/PMTWaveform.cc
  src/SimpleDAQProc.cc
  src/SplitEVDAQProc.cc
  src/ForcedTriggerProc.cc
  src/WaveformPrep.cc
  src/WaveformAnalyzerBase.cc
  src/WaveformAnalysisLognormal.cc
  src/WaveformAnalysisGaussian.cc
  src/WaveformAnalysisSinc.cc
  )

# Set our include directories
target_include_directories(daq SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(daq PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
