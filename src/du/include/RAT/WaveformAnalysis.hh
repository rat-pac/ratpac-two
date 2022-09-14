#ifndef __RAT_WaveformAnalysis__
#define __RAT_WaveformAnalysis__

#include <TObject.h>
#include <vector>

namespace RAT {

class WaveformAnalysis : public TObject {
public:
  WaveformAnalysis();
  virtual ~WaveformAnalysis();

  // Calculate baseline (in mV)
  double CalculatePedestal(std::vector<UShort_t> wfm, UShort_t low_window,
                           UShort_t high_window);

  // Linearly interpolate between two samples
  double Interpolate(double voltage1, double voltage2, double voltage_crossing,
                     double time_step);

  // Apply a constant fraction discriminator to
  // calculate the threshold crossing
  double CalculateTime(std::vector<UShort_t> wfm,
                       UShort_t low_window,  // sample #
                       UShort_t high_window, // sample #
                       double dy,            // mV/ADC
                       double sampling_rate, // GHz
                       double cfd_fraction = 0.60, UShort_t lookback = 30);

  UShort_t GetThresholdCrossing(std::vector<UShort_t> wfm, double dy,
                                double pedestal, double peak,
                                UShort_t peak_sample,
                                double cfd_fraction = 0.60,
                                UShort_t lookback = 30);

  // Calculate the peak (in mV) and corresponding sample
  void GetPeak(std::vector<UShort_t> wfm, double dy, double pedestal,
               double &peak, UShort_t &peak_sample);

  ClassDef(WaveformAnalysis, 0)

      protected :

      // Invalid value for bad waveforms
      UShort_t INVALID;
};

} // namespace RAT

#endif
