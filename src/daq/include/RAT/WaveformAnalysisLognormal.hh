////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisLognormal
///
/// \brief Apply lognormal fit to digitized waveforms
///
/// \author Tanner Kaptanoglu <tannerbk@berkeley.edu>
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
/// \author James Shen <jierans@sas.upenn.edu>
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
#include <RAT/WaveformAnalyzerBase.hh>
#include <vector>

namespace RAT {

class WaveformAnalysisLognormal : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisLognormal() : WaveformAnalysisLognormal("LognormalFit"){};
  WaveformAnalysisLognormal(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisLognormal", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisLognormal(){};
  void Configure(const std::string &config_name) override;
  virtual void SetD(std::string param, double value) override;

  // Fit the digitized waveform using a lognormal function
  void FitWaveform(const std::vector<double> &voltWfm);

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;

  // Analysis constants
  double fFitWindowLow;
  double fFitWindowHigh;
  double fFitShape;
  double fFitScale;

  // Coming from WaveformPrep
  double fDigitTimeInWindow;

  // Fitted variables
  double fFittedTime;
  double fFittedCharge;
  double fFittedBaseline;
  double fChi2NDF;

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm) override;
};

}  // namespace RAT

#endif
