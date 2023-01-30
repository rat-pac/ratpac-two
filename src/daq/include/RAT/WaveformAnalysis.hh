////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysis
///
/// \brief Processed the digitized waveforms
///
/// \author Tanner Kaptanoglu <tannerbk@berkeley.edu>
///
/// REVISION HISTORY:\n
///     25 Oct 2022: Initial commit
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform, providing tools to do timing at the
/// threshold crossing, integrated charge, etc.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysis__
#define __RAT_WaveformAnalysis__

#include <TObject.h>
#include <RAT/DS/PMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/DB.hh>

#include <vector>

namespace RAT {

class WaveformAnalysis {
 public:
  WaveformAnalysis();
  virtual ~WaveformAnalysis(){};

  void RunAnalysis(DS::PMT *pmt, int pmtID, Digitizer *fDigitizer);

  // Calculate baseline (in mV)
  double CalculatePedestal(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window);

  // Linearly interpolate between two samples
  double Interpolate(double voltage1, double voltage2, double voltage_crossing, double time_step);

  // Apply a constant fraction discriminator to
  // calculate the threshold crossing
  double CalculateTime(std::vector<UShort_t> wfm,
                       UShort_t low_window,   // sample #
                       UShort_t high_window,  // sample #
                       double dy,             // mV/ADC
                       double sampling_rate,  // GHz
                       double cfd_fraction = 0.60, UShort_t lookback = 30);

  // Find the sample where a threshold crossing occurs
  UShort_t GetThresholdCrossing(std::vector<UShort_t> wfm,
                                double dy,             // mV/ADC
                                double pedestal,       // ADC
                                double peak,           // mV
                                UShort_t peak_sample,  // sample #
                                double cfd_fraction = 0.60, UShort_t lookback = 30);

  // Calculate the peak (in mV) and corresponding sample
  void GetPeak(std::vector<UShort_t> wfm, double dy, double pedestal, double &peak, UShort_t &peak_sample);

  // Integrate the digitized waveform to calculate charge
  double Integrate(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window, double dy,
                   double sampling_rate, int integration_window_low, int integration_window_high,
                   double termination_ohms);

 protected:

  DBLinkPtr fDigit;
  int fPedWindowLow;
  int fPedWindowHigh;
  int fLookback;
  int fIntWindowLow;
  int fIntWindowHigh;
  double fConstFrac;

  // Invalid value for bad waveforms
  const UShort_t INVALID = 99999;
};

}  // namespace RAT

#endif
