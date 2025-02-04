////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisSPEMF
///
/// \brief Apply single PE matched fileter to digitized waveforms
///
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     22 Oct 2024: Initial commit
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform via a single PE matched filter.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisSPEMF__
#define __RAT_WaveformAnalysisSPEMF__

#include <TObject.h>
#include <TSpline.h>

#include <RAT/DB.hh>
#include <RAT/DS/Digit.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <vector>

namespace RAT {

class WaveformAnalysisSPEMF : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisSPEMF() : WaveformAnalysisSPEMF("SPEMatchedFilter"){};
  WaveformAnalysisSPEMF(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisSPEMF", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisSPEMF(){};
  void Configure(const std::string& config_name) override;
  virtual void SetD(std::string param, double value) override;

  void GetTemplatebyModelName(std::string modelName);
  double MatchedFilter(const std::vector<double>& voltWfm, const TSpline3* templateWfm, double tau,
                       const double template_delay);
  void FitWaveform(const std::vector<double>& voltWfm);

 protected:
  // Settings
  DBLinkPtr fDigit;
  double fTemplateDelay;
  int fUpsampleFactor;

  // Analysis constants
  std::string fModelName;
  std::string fPMTPulseType;
  std::string fPMTPulseShape;
  DBLinkPtr lpulse;

  std::vector<double> fPMTPulseShapeTimes;
  std::vector<double> fPMTPulseShapeValues;

  double fFitWindowLow;
  double fFitWindowHigh;

  // Coming from WaveformPrep
  double fDigitTimeInWindow;

  // Fitted variables
  double fFittedTime;
  double fMaxCorr;

  void DoAnalysis(DS::DigitPMT* pmt, const std::vector<UShort_t>& digitWfm);
};

}  // namespace RAT

#endif
