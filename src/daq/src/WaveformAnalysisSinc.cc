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
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
    fNumInterpPoints = fDigit->GetI("num_interp_points");
    fTaperingConst = fDigit->GetD("tapering_constant");
    fNumSincLobes = fDigit->GetI("num_sinc_lobes");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisSinc: Unable to find analysis parameters.");
  }
  calculateTSincKernel();
}

void WaveformAnalysisSinc::SetI(std::string param, int value) {
  if (param == "num_interp_points") {
    fNumInterpPoints = value;
  } else if (param == "num_sinc_lobes") {
    fNumSincLobes = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisSinc::SetD(std::string param, double value) {
  if (param == "fit_window_low") {
    fFitWindowLow = value;
  } else if (param == "fit_window_high") {
    fFitWindowHigh = value;
  } else if (param == "tapering_constant") {
    fTaperingConst = value;
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

  // check to see if digitTime is well behaved
  if (!(fDigitTimeInWindow > 0 && fDigitTimeInWindow < voltWfm.size() * fTimeStep)) {
    return;
  }

  InterpolateWaveform(voltWfm);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("Sinc");
  fit_result->AddPE(fFitTime, fFitCharge, {{"peak", fFitPeak}});
}

std::vector<double> WaveformAnalysisSinc::ConvolveWaveform(const std::vector<double>& wfm) {
  int wl = wfm.size();  // waveform length
  int numNewPoints = (wl - 1) * fNumInterpPoints + 1;

  std::vector<double> newWfm(numNewPoints, 0.0);  // vector to store the interpolated waveform
  for (int j = 0; j < wl - 1; j++) {              // looping over the digitized waveform
    for (int k = 0; k < fNumInterpPoints; k++)    // looping over the interpolated points
    {
      double interpValue = 0.;
      for (int i = 0; i < fNumSincLobes; i++)  // looping over the tsinc kernel
      {
        if (j - i >= 0) {
          interpValue += wfm[j - i] * tsinc_kernel[i * fNumInterpPoints + k];
        }
        if (j + i + 1 < wl) {
          interpValue += wfm[j + 1 + i] * tsinc_kernel[(i + 1) * fNumInterpPoints - k];
        }
      }
      newWfm[j * fNumInterpPoints + k] = interpValue;
    }
  }
  newWfm[numNewPoints - 1] =
      wfm[wl - 1];  // End point of the digitized waveform is the end point of the interpolated waveform
  return newWfm;
}

void WaveformAnalysisSinc::InterpolateWaveform(const std::vector<double>& voltWfm) {
  // Interpolation range
  double bf = fDigitTimeInWindow - fFitWindowLow;
  double tf = fDigitTimeInWindow + fFitWindowHigh;

  // Check the interpolation range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf >= voltWfm.size() * fTimeStep) ? (voltWfm.size() * fTimeStep) - (fTimeStep / 2.0) : tf;

  // Get samples values within the interpolation range
  std::vector<double> sampleWfm;
  int sampleLow = static_cast<int>(floor(bf / fTimeStep));
  int sampleHigh = static_cast<int>(floor(tf / fTimeStep));
  for (int j = sampleLow; j < sampleHigh + 1; j++) {
    sampleWfm.push_back(voltWfm[j]);
  }

  // Interpolated waveform
  std::vector<double> interpWfm = ConvolveWaveform(sampleWfm);
  std::pair<int, double> peakSampleVolt = WaveformUtil::FindHighestPeak(interpWfm);

  fFitPeak = peakSampleVolt.second;
  fFitTime = ((sampleLow + 0.5) * fTimeStep) + peakSampleVolt.first * fTimeStep / fNumInterpPoints;
  fFitCharge = 0;
  for (const double& vlt : interpWfm) {
    fFitCharge += WaveformUtil::VoltagetoCharge(vlt, fTimeStep / fNumInterpPoints, fTermOhms);  // in pC
  }
}

}  // namespace RAT
