////////////////////////////////////////////////////////////////////
/// \class RAT::Digitizer
///
/// \brief   Digitizer
///
/// \author Javier Caravaca <jcaravaca@berkeley.edu>
///
/// REVISION HISTORY:\n
///     1 Feb 2015: Initial commit
///
/// \details
/// This class provides full support for a CAEN digitizer.
/// It digitizes PMTWaveforms, check thresholds crossing,
/// integrate charge, calculate front-end times ...
////////////////////////////////////////////////////////////////////
#ifndef __RAT_Digitizer__
#define __RAT_Digitizer__

#include <RAT/DB.hh>
#include <RAT/DS/Digit.hh>
#include <RAT/Log.hh>
#include <RAT/PMTWaveform.hh>
#include <map>

namespace RAT {

class Digitizer {
 public:
  Digitizer(){};
  virtual ~Digitizer(){};
  Digitizer(std::string);

  virtual void SetDigitizerType(std::string);
  virtual void AddChannel(int ich, PMTWaveform wfm);

  std::string fDigitName;  // Digitizer type
  int fNBits;              // N bits of the digitizer
  double fVhigh;           // Upper dynamic range
  double fVlow;            // Lower dynamic range
  double fSamplingRate;    // Sampling rate in GHz
  int fNSamples;           // Total number of samples per digitized trace
  // Channel:Digitized waveform for each channel
  std::map<UShort_t, std::vector<UShort_t>> fDigitWaveForm;

  int fPedWindowLow;      // Lower edge of baseline window (samples)
  int fPedWindowHigh;     // Upper edge of baseline window (samples)
  double fConstFrac;      // Constant fraction of peak
  int fLookback;          // Number of samples to look back for thresh. crossing
  double fIntWindowLow;   // Lower edge of integration window, relative to thresh. crossing (ns)
  double fIntWindowHigh;  // Upper edge of integration window, relative to thresh. crossing (ns)

 protected:
  DBLinkPtr fLdaq;
  DBLinkPtr fDigit;

  double fOffset;     // Digitizer offset
  double fNoiseAmpl;  // Electronic noise width
};

}  // namespace RAT

#endif
