#ifndef __RAT_WaveformUtil__
#define __RAT_WaveformUtil__

#include <RtypesCore.h>

#include <tuple>
#include <vector>

namespace RAT {

namespace WaveformUtil {

// ADC counts to voltage (mV)
std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes);

// Calculate baseline (in mV)
double CalculatePedestal(const std::vector<double>& waveform, int pedWindowLow, int pedWindowHigh);

std::pair<int, double> FindHighestPeak(
    const std::vector<double>& voltageWaveform);  // Returns pair (peak_sample, peak_voltage)

// Find the sample where a threshold crossing occurs
int GetThresholdCrossing(const std::vector<double>& waveform, int peakSample, double voltageThreshold, int lookBack,
                         double timeStep);

// Find location and duration of each threshold crossing
std::tuple<int, double, double> GetNCrossings(
    const std::vector<double>& waveform, double voltageThreshold,
    double timeStep);  // Return tuple (nCrossings, time_over_threshold_voltage_over_threshold)

// Apply a constant fraction discriminator to calculate the threshold crossing
double CalculateTimeCFD(const std::vector<double>& waveform, std::pair<int, double> peak, double constFrac,
                        int lookBack, double timeStep);

// Integrate around highest peak
double IntegratePeak(const std::vector<double>& waveform, int peakSample, int intWindowLow, int intWindowHigh,
                     double timeStep, double termOhms);

// Integrate waveform in sliding windows
double IntegrateSliding(const std::vector<double>& waveform, int slidingWindow, double chargeThresh, double timeStep,
                        double termOhms);

}  // namespace WaveformUtil

}  // namespace RAT

#endif