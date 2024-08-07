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

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <vector>

namespace RAT {

class WaveformAnalysis {
 public:
  WaveformAnalysis();
  WaveformAnalysis(std::string analyzer_name);
  virtual ~WaveformAnalysis(){};

  void RunAnalysis(DS::DigitPMT *pmt, int pmtID, Digitizer *fDigitizer, double timeOffset = 0.0);
  double RunAnalysisOnTrigger(int pmtID, Digitizer *fDigitizer);

  // Calculate baseline (in mV)
  void CalculatePedestal();

  // Linearly interpolate between two samples
  void Interpolate(double voltage1, double voltage2);

  // Apply a constant fraction discriminator to
  // calculate the threshold crossing
  void CalculateTimeCFD();

  // Calculate the time a threshold crossing occurs, with a linear interpolation
  double CalculateThresholdCrossingTime();

  double CalculateThresholdCrossingTime(double voltage_threshold) {
    fVoltageCrossing = voltage_threshold;
    return CalculateThresholdCrossingTime();
  }

  // Find the sample where a threshold crossing occurs
  void GetThresholdCrossing();

  // Find the total number of threshold crossings
  void GetNCrossings();

  // Calculate the peak (in mV) and corresponding sample
  void GetPeak();

  // Integrate the digitized waveform to calculate charge
  void Integrate();

  // Integrate the digitized waveform to calculate charge
  void SlidingIntegral();

  // ADC counts to voltage (mV)
  double DigitToVoltage(UShort_t digit) { return (digit - fPedestal) * fVoltageRes; }

  // Fit the digitized waveform using a lognormal function
  void FitWaveform();

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;
  double fTimeStep;
  double fVoltageRes;
  double fTermOhms;

  // Analysis constants
  int fPedWindowLow;
  int fPedWindowHigh;
  double fLookback;
  int fIntWindowLow;
  int fIntWindowHigh;
  double fConstFrac;
  int fLowIntWindow;
  int fHighIntWindow;
  double fVoltageCrossing;
  double fThreshold;
  int fSlidingWindow;
  double fChargeThresh;
  double fFitWindowLow;
  double fFitWindowHigh;
  double fFitShape;
  double fFitScale;

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
  double fCharge;
  double fTotalCharge;
  double fVoltageOverThreshold;
  double fDigitTime;

  // Fitted variables
  int fRunFit;
  double fFittedTime;
  double fFittedHeight;
  double fFittedBaseline;
  double fChi2NDF;

  // Invalid value for bad waveforms
  const UShort_t INVALID = 9999;
};

}  // namespace RAT

#endif
