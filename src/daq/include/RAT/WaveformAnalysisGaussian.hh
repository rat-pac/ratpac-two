////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisGaussian
///
/// \brief Apply Gaussian fit to digitized waveforms
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform via a Gaussian fit.
/// Based off of WaveformAnalysisLognormal.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisGaussian__
#define __RAT_WaveformAnalysisGaussian__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <vector>

namespace RAT {

class WaveformAnalysisGaussian : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisGaussian() : WaveformAnalysisGaussian("GaussianFit"){};
  WaveformAnalysisGaussian(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisGaussian", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisGaussian(){};
  void Configure(const std::string &config_name) override;
  virtual void SetD(std::string param, double value) override;

  // Fit the digitized waveform using a Gaussian function
  void FitWaveform(const std::vector<double> &voltWfm);

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;

  // Analysis constants
  double fFitWindowLow;
  double fFitWindowHigh;
  double fFitWidth;
  double fFitWidthLow;
  double fFitWidthHigh;

  // Coming from WaveformPrep
  double fDigitTimeInWindow;

  // Fitted variables
  double fFittedTime;
  double fFittedCharge;
  double fFittedBaseline;
  double fFittedWidth;
  double fChi2NDF;

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm) override;
};

}  // namespace RAT

#endif
