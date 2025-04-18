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
///
/// Algorithm is from W. K. Warburton and W. Hennig,
/// "New Algorithms for Improved Digital Pulse Arrival Timing With
/// Sub-GSps ADCs," in IEEE Transactions on Nuclear Science, vol. 64,
/// no. 12, pp. 2938-2950, Dec. 2017, doi: 10.1109/TNS.2017.2766074.
///
/// A window for interpolation is chosen around the peak of the
/// digitized waveform. Interpolation is performed by convolving the
/// samples within this window and the tsinc kernel. The algorithm
/// for the discrete convolution code used here is similar to the
/// one specified in the paper (optimized to make use of sinc being
/// an even function).
///
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
  WaveformAnalysisSinc() : WaveformAnalysisSinc("TSincInterpolation"){};
  WaveformAnalysisSinc(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisSinc", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisSinc(){};
  void Configure(const std::string &config_name) override;
  virtual void SetI(std::string param, int value) override;
  virtual void SetD(std::string param, double value) override;

  // Interpolate the digitized waveform by convolution using a tapered sinc kernel
  std::vector<double> ConvolveWaveform(const std::vector<double> &wfm);
  void InterpolateWaveform(const std::vector<double> &voltWfm);

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;

  // Analysis constants
  double fFitWindowLow;
  double fFitWindowHigh;
  int fNumInterpPoints;   // number of interpolated points per data point (or the number of points in each lobe of
                          // the tsinc kernel)
  double fTaperingConst;  // tapering constant (determines how fast the kernel decays)
  int fNumSincLobes;      // number of sinc lobes to be included in the kernel (determines the length of the kernel)

  // Coming from WaveformPrep
  double fDigitTimeInWindow;

  // Fit variables
  double fFitTime;
  double fFitCharge;
  double fFitPeak;

  std::vector<double> tsinc_kernel;  // Stores the tsinc kernel
  void calculateTSincKernel() {      // Calculates the tsinc kernel
    for (int k = 0; k < fNumInterpPoints * fNumSincLobes + 1;
         k++) {  // Storing only the positive k values since sinc is an even function
      double val = k * M_PI / static_cast<float>(fNumInterpPoints);
      double sinc = sin(val);
      if (k == 0)
        sinc = 1.0;
      else
        sinc /= val;
      tsinc_kernel.push_back(sinc * exp(-pow(k / fTaperingConst, 2)));
    }
  }

  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm) override;
};

}  // namespace RAT

#endif
