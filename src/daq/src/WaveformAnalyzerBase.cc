#include "RAT/WaveformAnalyzerBase.hh"

#include "RAT/DS/DigitPMT.hh"

namespace RAT {

// Common features that need to be extracted from the analysis would go here. So far there's none.
void WaveformAnalyzerBase::Configure(const std::string& config_name) {}

void WaveformAnalyzerBase::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, Digitizer* fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  std::vector<UShort_t> digitWfm = fDigitizer->fDigitWaveForm[pmtID];
  fTermOhms = fDigitizer->fTerminationOhms;
  DoAnalysis(digitpmt, digitWfm);
}

void WaveformAnalyzerBase::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, DS::Digit* dsdigit) {
  fVoltageRes = dsdigit->GetVoltageResolution();
  fTimeStep = dsdigit->GetTimeStepNS();
  fTermOhms = dsdigit->GetTerminationOhms();
  std::vector<UShort_t> digitWfm = dsdigit->GetWaveform(pmtID);
  DoAnalysis(digitpmt, digitWfm);
}

void WaveformAnalyzerBase::SetS(std::string param, std::string value) {
  if (param == "analyzer_name") {
    Configure(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalyzerBase::SetD(std::string param, double value) { throw Processor::ParamUnknown(param); }
void WaveformAnalyzerBase::SetI(std::string param, int value) { throw Processor::ParamUnknown(param); }

}  // namespace RAT
