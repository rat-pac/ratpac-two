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
  src/WaveformAnalysisLucyDDM.cc
  src/WaveformAnalysisRAVEN.cc
  )
target_link_libraries(daq PUBLIC ratpac_common FFTW3::fftw3)

# Copy our headers when installing
install(DIRECTORY include/ DESTINATION include)
