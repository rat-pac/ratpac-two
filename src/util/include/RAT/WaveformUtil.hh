#ifndef __RAT_WaveformUtil__
#define __RAT_WaveformUtil__

#include <RtypesCore.h>

#include <tuple>
#include <vector>

namespace RAT {

namespace WaveformUtil {

// converts waveform from ADC counts to voltage (mV); expects pedestal in ADC counts if given
std::vector<double> ADCtoVoltage(const std::vector<UShort_t>& adcWaveform, double voltageRes, double pedestal = 0);

// Calculate baseline (in ADC counts or mV, depending on input)
double CalculatePedestal(const std::vector<UShort_t>& waveform, int pedWindowLow, int pedWindowHigh);
double CalculatePedestal(const std::vector<double>& waveform, int pedWindowLow, int pedWindowHigh);

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
    double timeStep);  // Return tuple (nCrossings, time_over_threshold_voltage_over_threshold)

// Apply a constant fraction discriminator to calculate the threshold crossing for a given peak
double CalculateTimeCFD(const std::vector<double>& waveform, std::pair<int, double> peak, double constFrac,
                        int lookBack, double timeStep);

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
