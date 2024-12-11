#ifndef __RAT_WaveformUtil__
#define __RAT_WaveformUtil__

#include <RtypesCore.h>

#include <tuple>
#include <vector>

namespace RAT {

namespace WaveformUtil {

const int INVALID = 9999;

// converts waveform from ADC counts to voltage (mV); expects pedestal in ADC counts if given
std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes, double pedestal = 0);

// Calculate baseline
template <typename T>
double CalculatePedestal(const std::vector<T>& waveform, size_t pedWindowLow, size_t pedWindowHigh) {
  /*
  Template: Calculate the baseline in the window between low - high samples.
  */

  double pedestal = 0;

  if (pedWindowLow >= waveform.size()) {
    Log::Die("WaveformUtil: Start of pedestal window must be before end of waveform.");
  } else if (pedWindowLow >= pedWindowHigh) {
    Log::Die("WaveformUtil: Start of pedestal window must be before end of pedestal window.");
  } else if (pedWindowHigh > waveform.size()) {
    Log::Die("WaveformUtil: End of pedestal window must be at most end of waveform.");
  }

  // Ensure end of pedestal window is less than waveform size
  pedWindowHigh = (pedWindowHigh > waveform.size()) ? waveform.size() : pedWindowHigh;

  for (size_t i = pedWindowLow; i < pedWindowHigh; i++) {
    pedestal += waveform[i];
  }
  pedestal /= (pedWindowHigh - pedWindowLow);
  return pedestal;
}

// Calculate baseline in ADC counts
inline double CalculatePedestalADC(const std::vector<UShort_t>& waveform, int pedWindowLow, int pedWindowHigh) {
  return CalculatePedestal<UShort_t>(waveform, pedWindowLow, pedWindowHigh);
};

// Calculate baseline in mV
inline double CalculatePedestalmV(const std::vector<double>& waveform, int pedWindowLow, int pedWindowHigh) {
  return CalculatePedestal<double>(waveform, pedWindowLow, pedWindowHigh);
};

std::pair<int, double> FindHighestPeak(
    const std::vector<double>& voltageWaveform);  // Returns pair (peak_sample, peak_voltage)

// Find the sample where a threshold crossing occurs before a given peak
int GetThresholdCrossingBeforePeak(const std::vector<double>& waveform, int peakSample, double voltageThreshold,
                                   int lookBack, double timeStep);

// Find total number of threshold crossings
int GetNCrossings(const std::vector<double>& waveform, double voltageThreshold);

// Find total number of threshold crossings, time over threshold, and voltage over threshold
std::tuple<int, double, double> GetCrossingsInfo(
    const std::vector<double>& waveform, double voltageThreshold,
    double timeStep);  // Returns tuple (nCrossings, time_over_threshold, voltage_over_threshold)

// Apply a constant fraction discriminator to calculate the threshold crossing for a given peak
double CalculateTimeCFD(const std::vector<double>& waveform, int peakSample, int lookBack, double timeStep,
                        double constFrac = INVALID, double voltageThreshold = INVALID);

// calculate charge (pC) from voltage (mV)
inline double VoltagetoCharge(double voltage, double timeStep, double termOhms) {
  return (-voltage * timeStep) / termOhms;
};

// Integrate around a peak to find charge
double IntegratePeak(const std::vector<double>& waveform, int peakSample, int intWindowLow, int intWindowHigh,
                     double timeStep, double termOhms);

// Integrate waveform in sliding windows to find total charge
double IntegrateSliding(const std::vector<double>& waveform, int slidingWindow, double chargeThresh, double timeStep,
                        double termOhms);

}  // namespace WaveformUtil

}  // namespace RAT

#endif
