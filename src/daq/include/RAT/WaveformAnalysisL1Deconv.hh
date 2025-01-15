////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisL1Deconv
///
/// \brief Apply L1 deconvolution to digitized waveforms
///
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     22 Oct 2024: Initial commit
///
/// \details
/// This class provides full support for analysis of the
/// digitized waveform via L1 deconvolution.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisL1Deconv__
#define __RAT_WaveformAnalysisL1Deconv__

#include <Math/Functor.h>
#include <Minuit2/Minuit2Minimizer.h>
#include <TSpline.h>
#include <omp.h>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <RAT/DB.hh>
#include <RAT/DS/Digit.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <memory>
#include <vector>

namespace RAT {

class WaveformAnalysisL1Deconv : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisL1Deconv() : WaveformAnalysisL1Deconv("L1Deconv"){};
  WaveformAnalysisL1Deconv(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisL1Deconv", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisL1Deconv(){};
  void Configure(const std::string& config_name) override;
  virtual void SetD(std::string param, double value) override;

 protected:
  // Settings
  DBLinkPtr fDigit;
  double fTemplateDelay;
  int fUpsampleFactor;
  double fNoiseSigma;
  double fPeakThreshold;      // Absolute threshold for peak detection
  double fRelativeThreshold;  // Relative threshold compared to nearest peaks
  double fTemplateHeight;     // Maximum height of template

  // Peak finding methods
  std::vector<double> FindPeaks(const Eigen::VectorXd& signal, double absThreshold, double relThreshold) const;

  // Analysis constants
  std::string fModelName;
  std::string fPMTPulseType;
  std::string fPMTPulseShape;
  DBLinkPtr lpulse;

  std::vector<double> fPMTPulseShapeTimes;
  std::vector<double> fPMTPulseShapeValues;

  // S matrix for deconvolution
  Eigen::MatrixXd sMatrix;
  bool sMatrixComputed = false;

  std::vector<double> initialParams;

  // Use smart pointer for templateSpline
  std::unique_ptr<TSpline3> templateSpline;

  // Method for L1 deconvolution
  Eigen::VectorXd L1Deconvolution(const Eigen::VectorXd& voltWfmVec, const Eigen::MatrixXd& sMatrix, double noiseSigma);

  // Method for performing the analysis
  void DoAnalysis(DS::DigitPMT* pmt, const std::vector<UShort_t>& digitWfm);
};

}  // namespace RAT

#endif  // WAVEFORMANALYSISL1DECONV_HH
