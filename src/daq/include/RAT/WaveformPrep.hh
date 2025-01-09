////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformPrep
///
/// \brief Process the digitized waveforms
///
/// \author Tanner Kaptanoglu <tannerbk@berkeley.edu>
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     25 Oct 2022: Initial commit
///     14 Oct 2024: Refactored from WaveformAnalysis to WaveformPrep
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform, providing tools to do timing at the
/// threshold crossing, integrated charge, etc.
/// Refactored from implementation in WaveformAnalysis.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformPrep__
#define __RAT_WaveformPrep__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <vector>

namespace RAT {

class WaveformPrep : public Processor {
 public:
  WaveformPrep();
  WaveformPrep(std::string analyzer_name);
  virtual ~WaveformPrep(){};

  void Configure(const std::string &analyzer_name);
  void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, Digitizer *fDigitizer, double timeOffset = 0.0);
  void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, DS::Digit *dsdigit, double timeOffset = 0.0);

  double RunAnalysisOnTrigger(int pmtID, Digitizer *fDigitizer);

  void ZeroSuppress(DS::EV *ev, DS::DigitPMT *digitpmt, int pmtID);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);
  virtual void SetS(std::string param, std::string value);
  virtual void SetD(std::string param, double value);
  virtual void SetI(std::string param, int value);

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

  // Use Cable offsets specified in channel status?
  int fApplyCableOffset;
  int fZeroSuppress;

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &DigitWfm, double timeOffset);
};

}  // namespace RAT

#endif
