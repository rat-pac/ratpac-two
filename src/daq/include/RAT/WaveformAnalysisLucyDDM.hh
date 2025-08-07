////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisLucyDDM
///
/// \brief Perform PMT waveform analysis using Richard-Lucy Direct Demodulation (LucyDDM).
///
/// \details
/// Reconstruct multiple photoelectrons in a single PMT waveform using direct deconvolution.
/// Method is mainly based on the discussion Sec. 3.3.2 of https://arxiv.org/abs/2112.06913
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisLucyDDM__
#define __RAT_WaveformAnalysisLucyDDM__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/FFTW1DTransformer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <vector>

namespace RAT {

class WaveformAnalysisLucyDDM : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisLucyDDM() : WaveformAnalysisLucyDDM("LucyDDM"){};
  WaveformAnalysisLucyDDM(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisLucyDDM", config_name) {
    Configure(config_name);
  };
  virtual ~WaveformAnalysisLucyDDM(){};
  void Configure(const std::string &config_name) override;
  // virtual void SetD(std::string param, double value) override;

 protected:
  // Digitizer settings
  DBLinkPtr fDigit;

  // shaping parameters for SPE waveform
  double vpe_scale;   // Lognormal `m`
  double vpe_shape;   // Lognormal `sigma`
  double vpe_charge;  // nominal charge of a PE.
  double vpe_integral =
      -9999;  // to be overwritten by the charge * termohms. Set to invalid here because termOhms is not known yet.
  double ds;  // internal sampling period in ns.
  size_t vpe_nsamples;  // number of samples in the VPE waveform.
  std::vector<double> vpe_norm,
      vpe_norm_flipped;  // single PE waveform and its flipped version. Both normalized to integral 1.
  std::vector<std::complex<double>> vpe_norm_fft, vpe_norm_flipped_fft;  // and their fourier transforms.

  double roi_threshold;    // Waveform below this threshold will be zeroed out.
  double epsilon = 1e-10;  // Small value to avoid division by zero in deconvolution

  size_t max_iterations;     // max iterations to run in deconvolution
  double stopping_nll_diff;  // Stop if the change in the poisson negative log-likelihood is less than this value

  double peak_height_threshold;  // peak height threshold for finding hits
  double charge_threshold;       // remove a hit if it has charge below this threshold.

  // FFT transfomration engines.
  std::unique_ptr<FFTW1DTransformer> fft = nullptr;
  std::unique_ptr<FFTW1DTransformer> ifft = nullptr;

  /// @brief Main analysis function entry point, provided by WaveformAnalyzerBase.
  void DoAnalysis(DS::DigitPMT *pmt, const std::vector<UShort_t> &digitWfm) override;

  /// @brief Ensure that hte requested size can be done with the existing transformers. If the transformers are too
  /// small or doesnt exist, create a new one that is big enough to acocmodate the request.
  void RequestFFTSize(size_t size);

  /// @brief convolve a real waveform with the FFT of a kernel.
  std::vector<double> ConvolveFFT(const std::vector<double> &a, const std::vector<std::complex<double>> &b,
                                  size_t conv_size, double dt) const;

  /**
   * @brief performs Richard-Lucy deconvolution of the waveform.
   * @param wfm waveform to deconvolve.
   * @out iterations_ran number of iterations ran in the deconvolution.
   * @return deconvolved waveform.
   * */
  std::vector<double> Deconvolve(const std::vector<double> &wfm, size_t &iterations_ran) const;

  /**
   * @brief Find hits in the deconvolved waveform.
   * @param phi deconvolved waveform.
   * @out out_times output hit times.
   * @out out_charges output hit charges.
   * @out out_time_errors output hit time errors, as reported by Minuit.
   * @out out_charge_errors output hit charge errors, as reported by Minuit.
   * @out chi2ndf output chi2/ndf of the fit.
   * */
  void FindHits(const std::vector<double> &phi, std::vector<double> &out_times, std::vector<double> &out_charges,
                std::vector<double> &out_time_errors, std::vector<double> &out_charge_errors, double &chi2ndf) const;
  void ClampBelowThreshold(std::vector<double> &wfm, double thresh = -9999) const;
  std::vector<double> ReblurWaveform(const std::vector<double> &phi) const;
  std::vector<double> Resample(const std::vector<double> &wfm, size_t n_samples) const;
  double PoissonNLL(const std::vector<double> &wfm, const std::vector<double> &reblurred_wfm) const;
  double GaussianPulseTrain(double *x, double *p, size_t N) const;
};

}  // namespace RAT

#endif
