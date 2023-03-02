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
  void CalculatePedestal();

  // Linearly interpolate between two samples
  void Interpolate(double voltage1, double voltage2);

  // Apply a constant fraction discriminator to
  // calculate the threshold crossing
  double CalculateTime();

  // Find the sample where a threshold crossing occurs
  void GetThresholdCrossing();

  // Find the total number of threshold crossings
  void GetNCrossings();

  // Calculate the peak (in mV) and corresponding sample
  void GetPeak();

  // Integrate the digitized waveform to calculate charge
  double Integrate();

 protected:

  // Digitizer settings
  DBLinkPtr fDigit;
  double fTimeStep;
  double fVoltageRes;
  double fTermOhms;

  // Analysis constants 
  int fPedWindowLow;
  int fPedWindowHigh;
  int fLookback;
  int fIntWindowLow;
  int fIntWindowHigh;
  double fConstFrac;
  int fLowIntWindow;
  int fHighIntWindow;
  double fVoltageCrossing;
  double fThreshold;

  // Digitized waveform
  std::vector<UShort_t> fDigitWfm;

  // Analysis variables
  double fInterpolatedTime;
  double fVoltagePeak;
  UShort_t fSamplePeak;
  double fPedestal;
  UShort_t fThresholdCrossing;
  UShort_t fNCrossings;
  double fTimeOverThreshold;

  // Invalid value for bad waveforms
  const UShort_t INVALID = 99999;
};

}  // namespace RAT

#endif
