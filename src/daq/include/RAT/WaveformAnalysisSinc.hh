////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisSinc
///
/// \brief Interpolate waveforms by convolution using sinc kernel
///
/// \author Yashwanth Bezawada <ysbezawada@berkeley.edu>
/// \author Tanner Kaptanoglu <tannerbk@berkeley.edu>
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
/// \author James Shen <jierans@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     25 Nov 2024: Initial commit
///
/// \details
/// This class provides full support for interpolating
/// the waveforms by convolution using a tapered sinc (tsinc)
/// kernel. Based on the implementation of the existing
/// WaveformAnalysis class and WaveformAnalysisLognormal class.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisSinc__
#define __RAT_WaveformAnalysisSinc__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <cmath>
#include <vector>

namespace RAT {

class WaveformAnalysisSinc : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisSinc() : WaveformAnalysisSinc(""){};
  WaveformAnalysisSinc(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisSinc", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisSinc(){};
  void Configure(const std::string &config_name) override;
  virtual void SetD(std::string param, double value) override;

  // Interpolate the digitized waveform by convolution using a sinc kernel
  std::vector<double> convolve_wfm(const std::vector<double> &wfm, const std::vector<double> &kernel);
  void InterpolateWaveform(const std::vector<double> &voltWfm);

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;

  // Analysis constants
  double fFitWindowLow;
  double fFitWindowHigh;

  // Coming from WaveformPrep
  double fDigitTimeInWindow;

  // Fit variables
  double fFitTime;
  double fFitCharge;
  double fFitPeak;

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm) override;
};

}  // namespace RAT

#endif
