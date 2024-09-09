#include <RAT/Log.hh>
#include <RAT/WaveformUtil.hh>

namespace RAT {

namespace WaveformUtil {

std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes) {
  std::vector<double> voltageWaveform;
  voltageWaveform.reserve(adcWaveform.size());

  for (UShort_t adcVal : adcWaveform) {
    voltageWaveform.push_back(adcVal * voltageRes);
  }

  return voltageWaveform;
}

double CalculatePedestal(const std::vector<double>& waveform, int pedWindowLow, int pedWindowHigh) {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  double pedestal = 0;

  if (pedWindowLow > waveform.size()) {
    Log::Die("Start of pedestal window must be before end of waveform.");
  } else if (pedWindowLow >= pedWindowHigh) {
    Log::Die("Start of pedestal window must be before end of pedestal window.");
  } else if (pedWindowHigh > waveform.size()) {
    Log::Die("End of pedestal window must be at most end of waveform.");
  }

  // Ensure end of pedestal window is less than waveform size
  pedWindowHigh = (pedWindowHigh > waveform.size()) ? waveform.size() : pedWindowHigh;

  for (int i = pedWindowLow; i < pedWindowHigh; i++) {
    pedestal += waveform[i];
  }
  pedestal /= (pedWindowHigh - pedWindowLow);
  return pedestal;
}

std::pair<int, double> FindHighestPeak(const std::vector<double>& voltageWaveform) {
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  double voltagePeak = 999;
  int samplePeak = -999;
  for (int i = 0; i < voltageWaveform.size(); i++) {
    double voltage = voltageWaveform[i];
    // Downward going pulse
    if (voltage < voltagePeak) {
      voltagePeak = voltage;
      samplePeak = i;
    }
  }
  return std::make_pair(samplePeak, voltagePeak);
}

int GetThresholdCrossing(const std::vector<double>& waveform, int peakSample, double voltageThreshold, int lookBack,
                         double timeStep) {
  /*
  Identifies the sample at which threshold crossing occurs
   */
  int thresholdCrossing = 0;
  // Make sure we don't scan passed the beginning of the waveform
  int lb = peakSample - int(lookBack / timeStep);
  int back_window = (lb > 0) ? lb : 0;

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
      thresholdCrossing = 9999;  // Invalid value for bad waveforms
      break;
    }
  }
  return thresholdCrossing;
}

std::tuple<int, double, double> GetNCrossings(const std::vector<double>& waveform, double voltageThreshold,
                                              double timeStep) {
  /*
  Calculates the total number of threshold crossings
  */
  int nCrossings = 0;
  double timeOverThreshold = 0;
  double voltageOverThreshold = 0;

  bool crossed = false;
  // Scan over the entire waveform
  for (int i = 0; i < waveform.size(); i++) {
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
double CalculateTimeCFD(const std::vector<double>& waveform, std::pair<int, double> peak, double constFrac,
                        int lookBack, double timeStep) {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */
  double voltageThreshold = constFrac * peak.second;
  int time = GetThresholdCrossing(waveform, peak.first, voltageThreshold, lookBack, timeStep);
  // Linearly interpolate threshold crossing time
  double deltav = waveform[time + 1] - waveform[time];
  double dt = timeStep / deltav * (voltageThreshold - waveform[time]);
  return time * timeStep + dt;
}

double IntegratePeak(const std::vector<double>& waveform, int peakSample, int intWindowLow, int intWindowHigh,
                     double timeStep, double termOhms) {
  /*
  Integrate the digitized waveform around the peak to calculate charge
  */
  double charge = 0;
  int windowStart = peakSample - intWindowLow;
  int windowEnd = peakSample + intWindowHigh;

  if (windowStart >= waveform.size()) {
    charge = 9999;  // Invalid value for bad waveforms
    return charge;
  }

  // Make sure not to integrate past the end of the waveform
  windowEnd = (windowEnd > waveform.size()) ? waveform.size() : windowEnd;
  // Make sure not to integrate before the waveform starts
  windowStart = (windowStart < 0) ? 0 : windowStart;

  for (int i = windowStart; i < windowEnd; i++) {
    double voltage = waveform[i];
    charge += (-voltage * timeStep) / termOhms;  // in pC
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
      charge += (-voltage * timeStep) / termOhms;  // in pC
    }
    if (charge > chargeThresh) {
      total_charge += charge;
    }
  }
  return total_charge;
}

}  // namespace WaveformUtil

}  // namespace RAT
