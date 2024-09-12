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
#include <RAT/DS/EV.hh>
#include <RAT/DS/MCPMT.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/PMTWaveformGenerator.hh>
#include <map>

namespace RAT {

class Digitizer {
 public:
  Digitizer(){};
  virtual ~Digitizer(){};
  Digitizer(std::string);

  virtual void SetDigitizerType(std::string);
  virtual void DigitizePMT(DS::MCPMT *mcpmt, int pmtID, double triggerTime, DS::PMTInfo *pmtinfo);
  virtual void ClearWaveforms();
  virtual void WriteToEvent(DS::EV *ev);
  virtual void AddChannel(int ichannel, PMTWaveform pmtwf);

  void AddWaveformGenerator(std::string modelName);

  std::string fDigitName;   // Digitizer type
  int fNBits;               // N bits of the digitizer
  double fVhigh;            // Upper dynamic range
  double fVlow;             // Lower dynamic range
  double fSamplingRate;     // Sampling rate in GHz
  int fNSamples;            // Total number of samples per digitized trace
  double fTerminationOhms;  // Input impedence of digitizer
  // Channel:Digitized waveform for each channel
  std::map<int, std::vector<UShort_t>> fDigitWaveForm;

  std::map<std::string, PMTWaveformGenerator *> fPMTWaveformGenerators;

 protected:
  DBLinkPtr fLdaq;
  DBLinkPtr fDigit;

  double fOffset;     // Digitizer offset
  double fNoiseAmpl;  // Electronic noise width
};

}  // namespace RAT

#endif
