////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisLognormal
///
/// \brief Apply lognormal fit to digitized waveforms
///
/// \author Tanner Kaptanoglu <tannerbk@berkeley.edu>
///
/// REVISION HISTORY:\n
///     25 Oct 2022: Initial commit
///     14 Oct 2024: Refactoring
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform via a lognormal fit.
/// Refactored from implementation in WaveformAnalysis.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisLognormal__
#define __RAT_WaveformAnalysisLognormal__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <vector>

namespace RAT {

class WaveformAnalysisLognormal : public Processor {
 public:
  WaveformAnalysisLognormal();
  WaveformAnalysisLognormal(std::string analyzer_name);
  virtual ~WaveformAnalysisLognormal(){};

  void Configure(const std::string &analyzer_name);
  void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, Digitizer *fDigitizer);
  void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, DS::Digit *dsdigit);

  // Fit the digitized waveform using a lognormal function
  void FitWaveform(const std::vector<double> &voltWfm);

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
  double fFitWindowLow;
  double fFitWindowHigh;
  double fFitShape;
  double fFitScale;

  // Analysis variables
  double fDigitTime;

  // Fitted variables
  double fFittedTime;
  double fFittedHeight;
  double fFittedBaseline;
  double fChi2NDF;

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm);
};

}  // namespace RAT

#endif
