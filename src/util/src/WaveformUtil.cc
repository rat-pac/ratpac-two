#include <RAT/Log.hh>
#include <RAT/WaveformUtil.hh>

namespace RAT {

namespace WaveformUtil {

std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes, double pedestal) {
  /*
  Convert a waveform from ADC units to voltage (mV), and optionally subtract a pedestal (given in ADC units)
  */
  std::vector<double> voltageWaveform;
  voltageWaveform.reserve(adcWaveform.size());

  for (UShort_t adcVal : adcWaveform) {
    voltageWaveform.push_back((adcVal - pedestal) * voltageRes);
  }

  return voltageWaveform;
}

std::pair<int, double> FindHighestPeak(const std::vector<double>& voltageWaveform) {
  /*
  Calculate the highest peak (in mV) and the corresponding sample.
  */
  double voltagePeak = INVALID;
  int samplePeak = INVALID;
  for (size_t i = 0; i < voltageWaveform.size(); i++) {
    double voltage = voltageWaveform[i];
    // Downward going pulse
    if (voltage < voltagePeak) {
      voltagePeak = voltage;
      samplePeak = i;
    }
  }
  return std::make_pair(samplePeak, voltagePeak);
}

int GetThresholdCrossingBeforePeak(const std::vector<double>& waveform, int peakSample, double voltageThreshold,
                                   int lookBack, double timeStep) {
  /*
  Identifies the sample at which threshold crossing occurs before a given peak
   */
  int thresholdCrossing = 0;
  // Make sure we don't scan past the beginning of the waveform
  int lb = peakSample - int(lookBack / timeStep);
  int back_window = (lb > 0) ? lb : 0;

  if (static_cast<size_t>(back_window) >= waveform.size()) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Start of lookback window not before end of waveform."
          << newline;
  } else if (back_window >= peakSample) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Start of lookback window not before peak." << newline;
  } else if (static_cast<size_t>(peakSample) >= waveform.size()) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Peak not before end of waveform." << newline;
  } else if (waveform.at(peakSample) > voltageThreshold) {
    debug << "WaveformUtil: Peak not above threshold.\n";
  }

  // Start at the peak and scan backwards
  for (int i = peakSample; i > back_window; i--) {
    double voltage = waveform[i];

    if (voltage > voltageThreshold) {
      thresholdCrossing = i;
      break;
    }

    // Reached the begining of the waveform
    // returned an invalid value
    if (i == back_window) {
      thresholdCrossing = INVALID;  // Invalid value for bad waveforms
      break;
    }
  }
  return thresholdCrossing;
}

int GetNCrossings(const std::vector<double>& waveform, double voltageThreshold) {
  /*
  Calculates the total number of threshold crossings
  */
  int nCrossings = 0;

  bool crossed = false;
  // Scan over the entire waveform
  for (size_t i = 0; i < waveform.size(); i++) {
    double voltage = waveform[i];

    // If we crossed below threshold
    if (voltage < voltageThreshold) {
      // Not already below thresh, count the crossing
      if (!crossed) {
        nCrossings += 1;
      }
      // Mark that we crossed
      crossed = true;
    }
    // If we are above threshold
    if (voltage > voltageThreshold) {
      crossed = false;
    }
  }
  return nCrossings;
}

std::tuple<int, double, double> GetCrossingsInfo(const std::vector<double>& waveform, double voltageThreshold,
                                                 double timeStep) {
  /*
  Calculates the total number of threshold crossings, time over threshold, and voltage over threshold
  */
  int nCrossings = 0;
  double timeOverThreshold = 0;
  double voltageOverThreshold = 0;

  bool crossed = false;
  // Scan over the entire waveform
  for (size_t i = 0; i < waveform.size(); i++) {
    double voltage = waveform[i];

    // If we crossed below threshold
    if (voltage < voltageThreshold) {
      // Not already below thresh, count the crossing
      if (!crossed) {
        nCrossings += 1;
      }
      // Count the time over threshold, mark that we crossed
      timeOverThreshold += timeStep;
      voltageOverThreshold += voltage;
      crossed = true;
    }
    // If we are above threshold
    if (voltage > voltageThreshold) {
      crossed = false;
    }
  }
  return std::make_tuple(nCrossings, timeOverThreshold, voltageOverThreshold);
}

double CalculateTimeCFD(const std::vector<double>& waveform, int peakSample, int lookBack, double timeStep,
                        double constFrac, double voltageThreshold) {
  /*
  Apply constant-fraction discriminator for a given peak
  */
  if (voltageThreshold == INVALID) {
    if (constFrac != INVALID) {
      voltageThreshold = constFrac * waveform.at(peakSample);
    } else {
      Log::Die("WaveformUtil::CalculateTimeCFD: Must give either constFrac or voltageThreshold for CalculateTimeCFD.");
    }
  }
  int time = GetThresholdCrossingBeforePeak(waveform, peakSample, voltageThreshold, lookBack, timeStep);
  if (time == INVALID) {
    return INVALID;
  }
  // Linearly interpolate threshold crossing time, if time is not last sample of waveform
  double dt = 0;
  if (time + 1 < static_cast<int>(waveform.size())) {
    double deltav = waveform.at(time + 1) - waveform.at(time);
    if (deltav != 0) {
      dt = (voltageThreshold - waveform.at(time)) / deltav;
    } else {
      dt = 0;
    }
  }
  return (time + dt) * timeStep;
}

double IntegratePeak(const std::vector<double>& waveform, int peakSample, int intWindowLow, int intWindowHigh,
                     double timeStep, double termOhms) {
  /*
  Integrate the digitized waveform around the given peak to calculate charge
  */
  double charge = 0;
  int windowStart = peakSample - intWindowLow;
  int windowEnd = peakSample + intWindowHigh;

  // Make sure not to integrate past the end of the waveform
  windowEnd = (static_cast<size_t>(windowEnd) > waveform.size()) ? waveform.size() : windowEnd;
  // Make sure not to integrate before the waveform starts
  windowStart = (windowStart < 0) ? 0 : windowStart;

  if (static_cast<size_t>(windowStart) >= waveform.size()) {
    charge = INVALID;  // Invalid value for bad waveforms
    return charge;
  }

  for (int i = windowStart; i < windowEnd; i++) {
    double voltage = waveform[i];
    charge += VoltagetoCharge(voltage, timeStep, termOhms);  // in pC
  }
  return charge;
}

double IntegrateSliding(const std::vector<double>& waveform, int slidingWindow, double chargeThresh, double timeStep,
                        double termOhms) {
  /*
  Integrate the digitized waveform over sliding windows to calculate a total charge
  */
  double total_charge = 0;
  int nsliding = int(waveform.size() / slidingWindow);

  for (int i = 0; i < nsliding; i++) {
    double charge = 0;
    int sample_start = i * slidingWindow;
    int sample_end = (i + 1) * slidingWindow;
    for (int j = sample_start; j < sample_end; j++) {
      double voltage = waveform[j];
      charge += VoltagetoCharge(voltage, timeStep, termOhms);  // in pC
    }
    if (charge > chargeThresh) {
      total_charge += charge;
    }
  }
  return total_charge;
}

}  // namespace WaveformUtil

}  // namespace RAT
