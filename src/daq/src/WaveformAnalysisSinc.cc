#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisSinc.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {
void WaveformAnalysisSinc::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    fPedWindowLow = fDigit->GetI("pedestal_window_low");
    fPedWindowHigh = fDigit->GetI("pedestal_window_high");
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisSinc: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisSinc::SetD(std::string param, double value) {
  if (param == "fit_window_low") {
    fFitWindowLow = value;
  } else if (param == "fit_window_high") {
    fFitWindowHigh = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisSinc::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisSinc: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);
  fDigitTimeInWindow = digitpmt->GetDigitizedTimeNoOffset();

  if (std::isinf(fDigitTimeInWindow) || fDigitTimeInWindow < 0)
  {
    return;
  }

  InterpolateWaveform(voltWfm);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("Sinc");
  fit_result->AddPE(fFitTime, fFitCharge, {{"peak", fFitPeak}});
}

std::vector<double> WaveformAnalysisSinc::convolve_wfm(const std::vector<double>& wfm, const std::vector<double>& kernel) {
  int n = wfm.size();
  int m = kernel.size();
  std::vector<double> result(n + m - 1, 0.0); // Size of the output
  // Perform the convolution
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      result[i + j] += wfm[i] * kernel[j];
    }
  }
  return result;
}

void WaveformAnalysisSinc::InterpolateWaveform(const std::vector<double>& voltWfm) {

  // Fit range
  double bf = fDigitTimeInWindow - fFitWindowLow;
  double tf = fDigitTimeInWindow + fFitWindowHigh;

  // Check the fit range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : tf;

  // Get samples values within the fit range
  std::vector<double> fit_wfm;
  int sample_low = static_cast<int>(floor(bf/fTimeStep));
  int sample_high = static_cast<int>(floor(tf/fTimeStep));
  sample_high = std::min(static_cast<int>(voltWfm.size()) - 1, sample_high);
  for (int j=sample_low; j<sample_high+1; j++){
    fit_wfm.push_back(voltWfm[j]);
  }

  // Calculating tapered sinc kernel
  std::vector<double> tsinc_kernel;
  int N = 8; //number of interpolated points per data point
  double T = 30.0; //tapering constant
  for (int k=-48; k<49; k++){
    double val = k*3.1415/(float)N;
    double sinc = sin(val);
    if (k == 0)
      sinc = 1.0;
    else
      sinc /= val;
    tsinc_kernel.push_back(sinc*exp(-pow(k/T,2)));
  }

  // Interpolated waveform
  std::vector<double> interp_wfm = convolve_wfm(fit_wfm, tsinc_kernel);
  std::pair<int, double> peakSampleVolt = WaveformUtil::FindHighestPeak(interp_wfm);

  fFitPeak = peakSampleVolt.second;
  fFitTime = bf + peakSampleVolt.first * fTimeStep / (float)N;
  fFitCharge = 0;
  for (auto & vlt : interp_wfm) {
    fFitCharge += WaveformUtil::VoltagetoCharge(vlt, fTimeStep / (float)N, fTermOhms);  // in pC
  }
}

}  // namespace RAT
