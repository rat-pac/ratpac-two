#include <RAT/Log.hh>
#include <RAT/WaveformAnalysis.hh>
#include <iostream>

namespace RAT {

WaveformAnalysis::WaveformAnalysis() : WaveformAnalysis::WaveformAnalysis("") {}

WaveformAnalysis::WaveformAnalysis(std::string analyzer_name) {
  fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", analyzer_name);
  fPedWindowLow = fDigit->GetI("pedestal_window_low");
  fPedWindowHigh = fDigit->GetI("pedestal_window_high");
  fLookback = fDigit->GetD("lookback");
  fIntWindowLow = fDigit->GetD("integration_window_low");
  fIntWindowHigh = fDigit->GetD("integration_window_high");
  fConstFrac = fDigit->GetD("constant_fraction");
  fThreshold = fDigit->GetD("voltage_threshold");
  fSlidingWindow = fDigit->GetD("sliding_window_width");
  fChargeThresh = fDigit->GetD("sliding_window_thresh");
}

void WaveformAnalysis::RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, Digitizer *fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  fDigitWfm = fDigitizer->fDigitWaveForm[pmtID];

  CalculatePedestal();

  double digit_time = CalculateTimeCFD();

  // Get the total number of threshold crossings
  GetNCrossings();

  fLowIntWindow = int((digit_time - fIntWindowLow) / fTimeStep);
  fHighIntWindow = int((digit_time + fIntWindowHigh) / fTimeStep);
  fTermOhms = fDigitizer->fTerminationOhms;

  Integrate();
  SlidingIntegral();

  digitpmt->SetDigitizedTime(digit_time);
  digitpmt->SetDigitizedCharge(fCharge);
  digitpmt->SetDigitizedTotalCharge(fTotalCharge);
  digitpmt->SetInterpolatedTime(fInterpolatedTime);
  digitpmt->SetSampleTime(fThresholdCrossing);
  digitpmt->SetNCrossings(fNCrossings);
  digitpmt->SetTimeOverThreshold(fTimeOverThreshold);
  digitpmt->SetVoltageOverThreshold(fVoltageOverThreshold);
  digitpmt->SetPedestal(fPedestal);
  digitpmt->SetPeakVoltage(fVoltagePeak);
}

double WaveformAnalysis::RunAnalysisOnTrigger(int pmtID, Digitizer *fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  fDigitWfm = fDigitizer->fDigitWaveForm[pmtID];

  CalculatePedestal();
  double trigger_threshold = fThreshold;
  double trigger_lookback = fLookback;
  try {
    trigger_threshold = fDigit->GetD("trigger_voltage_threshold");
    trigger_lookback = fDigit->GetD("trigger_lookback");
  } catch (DBNotFoundError &e) {
    warn << "WaveformAnalysis: Trigger threshold and lookback not found in database. "
         << "Using the same parameters as PMT Waveforms." << newline;
  }
  fVoltageRes *= -1;  // Invert the voltage since the waveform goes ABOVE threshold when a trigger occurs
  trigger_threshold *= -1;
  GetPeak();
  // HACK: Store the old lookback value, restore after a trigger time analysis is done.
  double old_lookback = fLookback;
  fLookback = trigger_lookback;
  double trigger_time = CalculateThresholdCrossingTime(trigger_threshold);
  fLookback = old_lookback;
  return trigger_time;
}

void WaveformAnalysis::CalculatePedestal() {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  fPedestal = 0;

  if (fPedWindowLow > fDigitWfm.size()) {
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than waveform size.");
  } else if (fPedWindowLow > fPedWindowHigh) {
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than end of pedestal window.");
  }

  // Ensure end of pedestal window is less than waveform size
  fPedWindowHigh = (fPedWindowHigh > fDigitWfm.size()) ? fDigitWfm.size() : fPedWindowHigh;

  for (UShort_t i = fPedWindowLow; i < fPedWindowHigh; i++) {
    fPedestal += fDigitWfm[i];
  }
  fPedestal /= (fPedWindowHigh - fPedWindowLow);
}

void WaveformAnalysis::Interpolate(double voltage1, double voltage2) {
  /*
  Linearly interpolate between two samples
  */
  double deltav = (voltage1 - voltage2);
  double dx = (fVoltageCrossing - voltage2) / deltav;
  fInterpolatedTime = dx * fTimeStep;
}

void WaveformAnalysis::GetPeak() {
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  fVoltagePeak = 999;
  fSamplePeak = -999;
  for (size_t i = 0; i < fDigitWfm.size(); i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);

    // Downward going pulse
    if (voltage < fVoltagePeak) {
      fVoltagePeak = voltage;
      fSamplePeak = i;
    }
  }
}

void WaveformAnalysis::GetThresholdCrossing() {
  /*
  Identifies the sample at which threshold crossing occurs
   */
  fThresholdCrossing = 0;
  // Make sure we don't scan passed the beginning of the waveform
  Int_t lb = Int_t(fSamplePeak) - Int_t(fLookback / fTimeStep);
  UShort_t back_window = (lb > 0) ? lb : 0;

  // Start at the peak and scan backwards
  for (UShort_t i = fSamplePeak; i > back_window; i--) {
    double voltage = (fDigitWfm[i] - fPedestal) * fVoltageRes;

    if (voltage > fVoltageCrossing) {
      fThresholdCrossing = i;
      break;
    }

    // Reached the begining of the waveform
    // returned an invalid value
    if (i == back_window) {
      fThresholdCrossing = INVALID;
      break;
    }
  }
}

void WaveformAnalysis::GetNCrossings() {
  /*
  Calculates the total number of threshold crossings
  */
  fNCrossings = 0;
  fTimeOverThreshold = 0;
  fVoltageOverThreshold = 0;

  bool fCrossed = false;
  // Scan over the entire waveform
  for (UShort_t i = 0; i < fDigitWfm.size(); i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);

    // If we crossed below threshold
    if (voltage < fThreshold) {
      // Not already below thresh, count the crossing
      if (!fCrossed) {
        fNCrossings += 1;
      }
      // Count the time over threshold, mark that we crossed
      fTimeOverThreshold += fTimeStep;
      fVoltageOverThreshold += voltage;
      fCrossed = true;
    }
    // If we are above threshold
    if (voltage > fThreshold) {
      fCrossed = false;
    }
  }
}

double WaveformAnalysis::CalculateThresholdCrossingTime() {
  /*
  Calculate the time a threshold crossing occurs, with a linear interpolation
  */
  // Get the sample where the voltage thresh is crossed
  GetThresholdCrossing();

  if (fThresholdCrossing == INVALID || fThresholdCrossing >= fDigitWfm.size()) {
    return INVALID;
  }

  if (fThresholdCrossing >= fDigitWfm.size()) {
    Log::Die("WaveformAnalysis: Threshold crossing sample larger than waveform window.");
  }

  // Interpolate between the two samples where the CFD threshold is crossed
  double v1 = DigitToVoltage(fDigitWfm[fThresholdCrossing + 1]);
  double v2 = DigitToVoltage(fDigitWfm[fThresholdCrossing]);
  Interpolate(v1, v2);

  double interpolated_time = double(fThresholdCrossing) * fTimeStep + fInterpolatedTime;

  return interpolated_time;
}

double WaveformAnalysis::CalculateTimeCFD() {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */

  // Calculate peak in mV
  GetPeak();
  fVoltageCrossing = fConstFrac * fVoltagePeak;
  return CalculateThresholdCrossingTime();
}

void WaveformAnalysis::Integrate() {
  /*
  Integrate the digitized waveform around the peak to calculate charge
  */
  fCharge = 0;

  if (fLowIntWindow >= fDigitWfm.size()) {
    fCharge = INVALID;
    return;
  }

  // Make sure not to integrate past the end of the waveform
  fHighIntWindow = (fHighIntWindow > fDigitWfm.size()) ? fDigitWfm.size() : fHighIntWindow;
  // Make sure not to integrate before the waveform starts
  fLowIntWindow = (fLowIntWindow < 0) ? 0 : fLowIntWindow;

  for (int i = fLowIntWindow; i < fHighIntWindow; i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);
    fCharge += (-voltage * fTimeStep) / fTermOhms;  // in pC
  }
}

void WaveformAnalysis::SlidingIntegral() {
  /*
  Integrate the digitized waveform over sliding windows to calculate a total charge
  */
  fTotalCharge = 0;
  int nsliding = int(fDigitWfm.size() / fSlidingWindow);

  for (int i = 0; i < nsliding; i++) {
    double charge = 0;
    int sample_start = i * fSlidingWindow;
    int sample_end = (i + 1) * fSlidingWindow;
    for (int j = sample_start; j < sample_end; j++) {
      double voltage = DigitToVoltage(fDigitWfm[j]);
      charge += (-voltage * fTimeStep) / fTermOhms;  // in pC
    }
    if (charge > fChargeThresh) {
      fTotalCharge += charge;
    }
  }
}

}  // namespace RAT
