#include <RAT/WaveformAnalysis.hh>
#include <iostream>

namespace RAT {

WaveformAnalysis::WaveformAnalysis() { INVALID = 9999; }

WaveformAnalysis::~WaveformAnalysis() {}

double WaveformAnalysis::CalculatePedestal(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window) {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  double pedestal = 0;

  for (UShort_t i = low_window; i < high_window; i++) {
    pedestal += wfm[i];
  }
  pedestal /= (high_window - low_window);

  return pedestal;
}

double WaveformAnalysis::Interpolate(double voltage1, double voltage2, double voltage_crossing, double time_step) {
  /*
  Linearly interpolate between two samples
  */
  double deltav = (voltage1 - voltage2);
  double dx = (voltage_crossing - voltage2) / deltav;
  double dt = dx * time_step;

  return dt;
}

void WaveformAnalysis::GetPeak(std::vector<UShort_t> wfm, double dy, double pedestal, double &peak,
                               UShort_t &peak_sample) {
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  for (size_t i = 0; i < wfm.size(); i++) {
    double voltage = (wfm[i] - pedestal) * dy;

    // Downward going pulse
    if (voltage < peak) {
      peak = voltage;
      peak_sample = i;
    }
  }
}

UShort_t WaveformAnalysis::GetThresholdCrossing(std::vector<UShort_t> wfm, double dy, double pedestal, double peak,
                                                UShort_t peak_sample, double cfd_fraction, UShort_t lookback) {
  /*
   */
  UShort_t threshold_crossing_sample = 0;
  double voltage_crossing = cfd_fraction * peak;

  // Start at the peak and scan backwards
  for (UShort_t i = peak_sample; i > peak_sample - lookback; i--) {
    double voltage = (wfm[i] - pedestal) * dy;

    if (voltage > voltage_crossing) {
      threshold_crossing_sample = i;
      break;
    }

    // Reached the begining of the waveform
    // returned an invalid value
    if (i == 0) return INVALID;
  }

  return threshold_crossing_sample;
}

double WaveformAnalysis::CalculateTime(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window, double dy,
                                       double sampling_rate, double cfd_fraction, UShort_t lookback) {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */

  // Calculate baseline in mV
  double pedestal = CalculatePedestal(wfm, low_window, high_window);

  // Calculate peak in mV
  double peak = 9999;
  UShort_t peak_sample = 0;
  GetPeak(wfm, dy, pedestal, peak, peak_sample);

  UShort_t threshold_crossing_sample =
      GetThresholdCrossing(wfm, dy, pedestal, peak, peak_sample, cfd_fraction, lookback);

  double time_step = 1.0 / sampling_rate;  // in ns

  // Interpolate between the two samples
  // where the CFD threshold is crossed
  double voltage_crossing = cfd_fraction * peak;
  double v1 = (wfm[threshold_crossing_sample + 1] - pedestal) * dy;
  double v2 = (wfm[threshold_crossing_sample] - pedestal) * dy;
  double dt = Interpolate(v1, v2, voltage_crossing, time_step);

  double tcdf = double(threshold_crossing_sample) * time_step + dt;

  return tcdf;
}
}  // namespace RAT
