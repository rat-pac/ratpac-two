#include <fftw3.h>
#include <sys/types.h>

#include <RAT/Log.hh>
#include <RAT/WaveformUtil.hh>

namespace RAT {

namespace WaveformUtil {

std::vector<double> ADCtoVoltage(const std::vector<UShort_t> &adcWaveform, double voltageRes, double pedestal) {
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

std::pair<int, double> FindHighestPeak(const std::vector<double> &voltageWaveform) {
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

int GetThresholdCrossingBeforePeak(const std::vector<double> &waveform, int peakSample, double voltageThreshold,
                                   int lookBack, double timeStep) {
  /*
  Identifies the sample at which threshold crossing occurs before a given peak
   */
  int thresholdCrossing = INVALID;
  // Make sure we don't scan past the beginning of the waveform
  int lb = peakSample - int(lookBack / timeStep);
  int back_window = (lb > 0) ? lb : 0;
  // No threshold crossing if highest peak is below threshold
  if (waveform.at(peakSample) > voltageThreshold) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Peak not above threshold.\n";
    return INVALID;
  }

  if (static_cast<size_t>(back_window) >= waveform.size()) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Start of lookback window not before end of waveform."
          << newline;
  } else if (back_window >= peakSample) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Start of lookback window not before peak." << newline;
  } else if (static_cast<size_t>(peakSample) >= waveform.size()) {
    debug << "WaveformUtil::GetThresholdCrossingBeforePeak: Peak not before end of waveform." << newline;
  }

  // Start at the peak and scan backwards
  for (int i = peakSample; i >= back_window; i--) {
    double voltage = waveform[i];

    if (voltage > voltageThreshold) {
      thresholdCrossing = i;
      break;
    }
  }
  return thresholdCrossing;
}

int GetNCrossings(const std::vector<double> &waveform, double voltageThreshold) {
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

std::tuple<int, double, double> GetCrossingsInfo(const std::vector<double> &waveform, double voltageThreshold,
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

double CalculateTimeCFD(const std::vector<double> &waveform, int peakSample, int lookBack, double timeStep,
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
    // If we didn't find threshold crossing but we also weren't able to scan the entire lookback range
    // because we reached the beginning of the waveform, return 0 instead of INVALID... and don't interpolate.
    if (peakSample - int(lookBack / timeStep) < 0) {
      return 0;
    }
    return INVALID;
  }
  // Linearly interpolate threshold crossing time, if time is not last sample of waveform
  double dt = 0;
  if (time + 1 < static_cast<int>(waveform.size())) {
    double deltav = waveform.at(time + 1) - waveform.at(time);
    dt = (deltav == 0) ? 0 : (voltageThreshold - waveform.at(time)) / deltav;
    if (dt < 0) {
      debug << "WaveformUtil::CalculateTimeCFD: Interpolating to value before threshold crossing. "
            << "This should not happen." << newline;
      return INVALID;
    }
  }
  return (time + dt) * timeStep;
}

double IntegratePeak(const std::vector<double> &waveform, int peakSample, int intWindowLow, int intWindowHigh,
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

double IntegrateSliding(const std::vector<double> &waveform, int slidingWindow, double chargeThresh, double timeStep,
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

static constexpr size_t next_power_of_two(size_t n) {
  size_t power = 1;
  while (power < n) power <<= 1;
  return power;
}
std::vector<double> ConvolveFFT(const std::vector<double> &a, const std::vector<double> &b, double dt) {
  size_t n = a.size();
  size_t m = b.size();
  size_t conv_size = n + m - 1;
  size_t fft_size = next_power_of_two(conv_size);  // pad to power of two for efficiency

  // Allocate input/output arrays
  std::vector<double> in1(fft_size, 0.0), in2(fft_size, 0.0);
  fftw_complex *out1 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (fft_size / 2 + 1));
  fftw_complex *out2 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (fft_size / 2 + 1));
  fftw_complex *product = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (fft_size / 2 + 1));
  std::vector<double> result(fft_size, 0.0);

  // Copy input
  std::copy(a.begin(), a.end(), in1.begin());
  std::copy(b.begin(), b.end(), in2.begin());

  // Create plans
  fftw_plan plan_fwd1 = fftw_plan_dft_r2c_1d(fft_size, in1.data(), out1, FFTW_ESTIMATE);
  fftw_plan plan_fwd2 = fftw_plan_dft_r2c_1d(fft_size, in2.data(), out2, FFTW_ESTIMATE);
  fftw_plan plan_inv = fftw_plan_dft_c2r_1d(fft_size, product, result.data(), FFTW_ESTIMATE);

  // Forward FFTs
  fftw_execute(plan_fwd1);
  fftw_execute(plan_fwd2);

  // Multiply in frequency domain
  for (size_t i = 0; i < fft_size / 2 + 1; ++i) {
    double re = out1[i][0] * out2[i][0] - out1[i][1] * out2[i][1];
    double im = out1[i][0] * out2[i][1] + out1[i][1] * out2[i][0];
    product[i][0] = re;
    product[i][1] = im;
  }

  // Inverse FFT
  fftw_execute(plan_inv);

  // Normalize
  for (double &x : result) x *= (dt / fft_size);

  // Cleanup
  fftw_destroy_plan(plan_fwd1);
  fftw_destroy_plan(plan_fwd2);
  fftw_destroy_plan(plan_inv);
  fftw_free(out1);
  fftw_free(out2);
  fftw_free(product);

  // Trim to actual convolution size
  result.resize(conv_size);
  return result;
}

std::vector<size_t> FindPeaks(const std::vector<double> &wfm, double threshold, int peak_direction) {
  if (peak_direction == 0) {
    Log::Die("WaveformUtil::FindPeaks: peak_direction must be non-zero.");
  }
  peak_direction = (peak_direction > 0) ? 1 : -1;  // Ensure direction is either 1 or -1
  double signed_threshold = peak_direction * threshold;
  std::vector<size_t> peaks;
  if (wfm.size() < 3) {
    warn << "WaveformUtil::FindPeaks: Not enough data points to find peaks." << newline;
    return peaks;
  }
  for (size_t idx = 1; idx < wfm.size() - 1; ++idx) {
    double mid = peak_direction * wfm[idx];
    double left = peak_direction * wfm[idx - 1];
    double right = peak_direction * wfm[idx + 1];
    if (mid > left && mid > right && mid > signed_threshold) {
      peaks.push_back(idx);
    }
  }
  return peaks;
}

}  // namespace WaveformUtil

}  // namespace RAT
