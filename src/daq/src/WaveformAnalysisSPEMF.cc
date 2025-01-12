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
    upsample_factor = fDigit->GetD("upsample_factor");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisSPEMF: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisSPEMF::SetD(std::string param, double value) {
  if (param == "template_delay") {
    fTemplateDelay = value;
  } else if (param == "upsample_factor") {
    upsample_factor = value;
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
  TSpline3* templateSpline =
      new TSpline3("template", fPMTPulseShapeTimes.data(), fPMTPulseShapeValues.data(), fPMTPulseShapeTimes.size());
  // Apply matched filter
  std::vector<double> corr = MatchedFilter(voltWfm, *templateSpline, upsample_factor = upsample_factor);

  // Find the maximum correlation value
  auto max_it = std::max_element(corr.begin(), corr.end());
  double max_corr = *max_it;
  size_t max_index = std::distance(corr.begin(), max_it);

  // Calculate the time corresponding to the maximum correlation
  double max_time = max_index / upsample_factor * fTimeStep;

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("SPEMatchedFilter");
  fit_result->AddPE(max_time, 1, {{"max_correlation", max_corr}});
}

std::vector<double> WaveformAnalysisSPEMF::MatchedFilter(const std::vector<double>& voltWfm,
                                                         const TSpline3 templateSpline, const int template_delay,
                                                         const int upsample_factor) {
  // Number of samples in the waveform
  size_t nsamples = voltWfm.size();
  std::vector<double> samples(nsamples);

  // Fill the sample indices
  for (size_t i = 0; i < nsamples; ++i) {
    samples[i] = i;
  }

  const size_t nupsampled = nsamples * upsample_factor;

  // Generate delay values for interpolation
  std::vector<double> delays(nupsampled);
  for (size_t i = 0; i < nupsampled; ++i) {
    delays[i] = static_cast<double>(i) / upsample_factor;
  }

  // Adjust for the template delay
  // Delays will be shifted to align the "zero" point of the template
  // for (auto& delay : delays) {
  //    delay -= template_delay;
  //}

  // Compute the matched filter correlation
  std::vector<double> corr(nupsampled, 0.0);
  for (size_t i = 0; i < nupsampled; ++i) {
    std::vector<double> shifted_template(nsamples, 0.0);
    for (size_t j = 0; j < nsamples; ++j) {
      double shifted_index = samples[j] + delays[i];
      if (shifted_index >= samples.front() && shifted_index <= samples.back()) {
        shifted_template[j] = templateSpline.Eval(shifted_index);
      } else {
        shifted_template[j] = 0.0;  // Assign zero if out of range
      }
    }
    // Compute dot product between waveform and shifted template
    corr[i] = std::inner_product(voltWfm.begin(), voltWfm.end(), shifted_template.begin(), 0.0);
  }

  return corr;
}

}  // namespace RAT