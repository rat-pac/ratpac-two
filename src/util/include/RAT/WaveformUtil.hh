#ifndef __RAT_WaveformUtil__
#define __RAT_WaveformUtil__

#include <tuple>
#include <vector>

namespace RAT {

// ADC counts to voltage (mV)
std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes);

// Calculate baseline (in mV)
double CalculatePedestal(const std::vector<double>& waveform, int pedWindowLow, int pedWindowHigh);

std::pair<int, double> FindHighestPeak(
    const std::vector<double>& voltageWaveform);  // Returns pair (peak_sample, peak_voltage)

// Find the total number of threshold crossings
int GetThresholdCrossing(const std::vector<double>& waveform, int peak, double voltageThreshold, double lookBack,
                         double timeStep);

std::tuple<int, double, double> GetNCrossings(const std::vector<double>& waveform, double voltageThreshold,
                                              double timeStep);

// Integrate waveform in sliding windows
double IntegrateSliding(const std::vector<double>& waveform, int slidingWindow, double chargeThresh, double timeStep,
                        double termOhms);

// Integrate around highest peak
double IntegratePeak(const std::vector<double>& waveform, int intWindowLow, int intWindowHigh, double timeStep,
                     double termOhms);

}  // namespace RAT

#endif