#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisSPEMF.hh>
#include <algorithm>
#include <numeric>

#include "RAT/DS/PMTInfo.hh"
#include "RAT/DS/RunStore.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {
void WaveformAnalysisSPEMF::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
    fTemplateDelay = fDigit->GetD("template_delay");
    fPMTPulseShapeTimes = fDigit->GetDArray("template_samples");
    fPMTPulseShapeValues = fDigit->GetDArray("template_values");
    fUpsampleFactor = fDigit->GetD("upsample_factor");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisSPEMF: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisSPEMF::SetD(std::string param, double value) {
  if (param == "template_delay") {
    fTemplateDelay = value;
  } else if (param == "upsample_factor") {
    fUpsampleFactor = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisSPEMF::GetTemplatebyModelName(std::string modelName) {
  // PMT pulse model specification
  try {
    lpulse = DB::Get()->GetLink("PMTPULSE", modelName);
    lpulse->GetS("index");
    info << "WaveformAnalysisSPEMF: Found pulse table for " << modelName << "." << newline;
  } catch (DBNotFoundError& e) {
    info << "WaveformAnalysisSPEMF: Could not find pulse table for " << modelName << ". Trying default." << newline;
    try {
      lpulse = DB::Get()->GetLink("PMTPULSE", "");
      lpulse->GetS("index");
      info << "WaveformAnalysisSPEMF: Using default pulse table for " << modelName << "." << newline;
    } catch (DBNotFoundError& e) {
      Log::Die("WaveformAnalysisSPEMF: No default pulse table found.");
    }
  }

  fPMTPulseShapeTimes = lpulse->GetDArray("pulse_shape_times");
  fPMTPulseShapeValues = lpulse->GetDArray("pulse_shape_values");
}

void WaveformAnalysisSPEMF::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisLognormal: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);
  fDigitTimeInWindow = digitpmt->GetDigitizedTimeNoOffset();
  // Fit waveform with matched filter
  FitWaveform(voltWfm);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("SPEMatchedFilter");
  fit_result->AddPE(fFittedTime, 1, {{"max_correlation", fMaxCorr}});
}

double WaveformAnalysisSPEMF::MatchedFilter(const std::vector<double>& voltWfm, const TSpline3* templateWfm, double tau,
                                            const double template_delay) {
  const size_t nsamples = voltWfm.size();
  double delay = tau - template_delay;
  double corr = 0.0;
  for (size_t i = 0; i < nsamples; i++) {
    if (i - delay >= 0 && i - delay < nsamples) {
      corr += voltWfm[i] * templateWfm->Eval(i - delay);
    } else {
      corr += 0.0;
    }
  }

  return corr;
}

void WaveformAnalysisSPEMF::FitWaveform(const std::vector<double>& voltWfm) {
  double bf = (fDigitTimeInWindow - fFitWindowLow) / fTimeStep;
  double tf = (fDigitTimeInWindow + fFitWindowHigh) / fTimeStep;

  // Check the fit range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf > voltWfm.size()) ? voltWfm.size() : tf;

  // Create a spline from the template
  TSpline3* templateSpline =
      new TSpline3("template", fPMTPulseShapeTimes.data(), fPMTPulseShapeValues.data(), fPMTPulseShapeTimes.size());

  // Apply matched filter
  fMaxCorr = 0;
  for (double i = bf * fUpsampleFactor; i < tf * fUpsampleFactor; i++) {
    double delay = i / fUpsampleFactor;
    double corr = MatchedFilter(voltWfm, templateSpline, delay, fTemplateDelay);
    if (corr > fMaxCorr) {
      fFittedTime = delay * fTimeStep;
      fMaxCorr = corr;
    }
  }

  delete templateSpline;
}

}  // namespace RAT