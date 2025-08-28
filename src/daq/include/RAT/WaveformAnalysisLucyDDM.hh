////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisLucyDDM
///
/// \brief Perform PMT waveform analysis using Richard-Lucy Direct Demodulation (LucyDDM).
///
/// \details
/// Reconstruct multiple photoelectrons in a single PMT waveform using direct deconvolution.
/// Method is mainly based on the discussion Sec. 3.3.2 of https://arxiv.org/abs/2112.06913
///
/// The waveform analysis is carreid out in three stages:
/// 1) Deconvolution. Utilyzing Richard-Lucy Direct Demoulation to deconvolve the waveform using a lognormal single PE
/// kernel.
/// 2) Wave packect identification. In the decononvovled waveform, determine local maximum as the peaks of wave packets.
/// Perform a gaussian fit around each peak to determine the time and charge of each wave packet.
/// 3) (optional) NPE estimation. If enabled, use a gaussian single-PE charge distribution PDF to estimate the number of
/// PEs in each resolved wave packet.
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

  bool npe_estimate;  // if true, perform a final NPE estimation on all resolved wave packet in the deconvolved
                      // waveform. Estimate the number of PEs in each packet using a gaussian PDF on charge.
                      // The mean of the single PE charge distribution is centered around `vpe_charge`.
  bool npe_estimate_charge_width;  // the width of the gaussian single-PE charge distribution.
  size_t npe_estimate_max_pes;     // upper limit for the number of PEs in a single resolved wave packet.

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

  /**
   * @brief Estimate the number of PEs in a resolved wave packet using a gaussian single-PE charge PDF.
   * @return estimated number of PEs.
   * */
  size_t EstimateNPE(double charge) const;

  /**
   * @brief Clamp all samples of a waveform below a threashold to epsilon (a positive value close to zero).
   *
   * @param wfm waveform to clamp (modified in place).
   * @param thresh threshold value. If a sample is below this value, it will be set to epsilon. If thresh is not
   * specified, use `roi_threshold`.
   * */
  void ClampBelowThreshold(std::vector<double> &wfm, double thresh = -9999) const;

  /**
   * @brief convolve `phi` with the SPE template, effectively "reblurring" the deconvolved waveform.
   * @param phi deconvolved waveform.
   * @return reblurred waveform.
   * */
  std::vector<double> ReblurWaveform(const std::vector<double> &phi) const;

  /**
   * @brief Resample a waveform to a different sampling period using linear interpolation.
   * @param wfm waveform to resample.
   * @param n_samples number of samples in the resampled waveform.
   * */
  std::vector<double> Resample(const std::vector<double> &wfm, size_t n_samples) const;

  /**
   * A Poisson negative log-likelihood between the original waveform and the reblurred waveform. Used as a cost function
   * to evaluate the quality of the deconvolution.
   * @param wfm original waveform.
   * @param reblurred_wfm deconvolved waveform, re-blurred by the spingle PE template.
   * */
  double PoissonNLL(const std::vector<double> &wfm, const std::vector<double> &reblurred_wfm) const;

  /**
   * Fit function that represents a series of gaussian pulses. To be fed to ROOT for minimization.
   * The parameters are in groups of 3, representing the amplitude, time, and width
   * */
  double GaussianPulseTrain(double *x, double *p, size_t N) const;
};

}  // namespace RAT

#endif
