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
    RAT::Log::Die("WaveformAnalysisSPEMF: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);

  // Compute the lookup table if it hasn't been computed yet
  if (!lookupTableComputed) {
    TSpline3* templateSpline =
        new TSpline3("template", fPMTPulseShapeTimes.data(), fPMTPulseShapeValues.data(), fPMTPulseShapeTimes.size());

    const size_t nsamples = voltWfm.size();
    const size_t nupsampled = nsamples * fUpsampleFactor;
    spline_values.resize(nsamples, std::vector<double>(nupsampled, 0.0));
    for (size_t j = 0; j < nsamples; ++j) {
      for (size_t i = 0; i < nupsampled; ++i) {
        double shifted_index = j - (static_cast<double>(i) / fUpsampleFactor - fTemplateDelay);
        if (shifted_index >= 0.0 && shifted_index < static_cast<double>(nsamples)) {
          spline_values[j][i] = templateSpline->Eval(shifted_index);
        }
      }
    }
    lookupTableComputed = true;

    // Clean up
    delete templateSpline;
  }

  // Apply matched filter
  std::vector<double> corr = MatchedFilter(voltWfm, spline_values, fTemplateDelay, fUpsampleFactor);

  // Find the maximum correlation value
  auto max_it = std::max_element(corr.begin(), corr.end());
  double max_corr = *max_it;
  size_t max_idx = std::distance(corr.begin(), max_it);

  // Calculate the time corresponding to the maximum correlation
  double max_time = (static_cast<double>(max_idx) / fUpsampleFactor - fTemplateDelay) * fTimeStep;

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("SPEMatchedFilter");
  fit_result->AddPE(max_time, 1, {{"max_correlation", max_corr}});
}

std::vector<double> WaveformAnalysisSPEMF::MatchedFilter(const std::vector<double>& voltWfm,
                                                         const std::vector<std::vector<double>>& spline_values,
                                                         const int template_delay, const int upsample_factor) {
  const size_t nsamples = voltWfm.size();
  const size_t nupsampled = nsamples * upsample_factor;
  std::vector<double> corr(nupsampled, 0.0);

  // Compute correlation using the precomputed spline values
  for (size_t i = 0; i < nupsampled; ++i) {
    double sumVal = 0.0;
    for (size_t j = 0; j < nsamples; ++j) {
      sumVal += voltWfm[j] * spline_values[j][i];
    }
    corr[i] = sumVal;
  }

  return corr;
}

}  // namespace RAT