////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisFSMP
///
/// \brief Fast (Stochastic) Matching Pursuit waveform analysis.
///
/// \details
/// Reconstructs photoelectron (PE) times and charges from a digitized PMT
/// waveform following the Fast Stochastic Matching Pursuit (FSMP) method of
/// Xu et al. 2022 (JINST 17 P06040, arXiv:2112.06913), the algorithm used by
/// the JUNO experiment.
///
/// The waveform is modelled as a sparse spike train of PEs convolved with a
/// single-PE response (SER) template plus Gaussian white noise (paper eq. 2.5).
/// A dictionary of time-shifted SER templates on an upsampled grid spans the
/// candidate PE times. For a given PE configuration `z` (which grid points host
/// a PE), the per-PE charges are integrated out analytically, yielding a
/// multivariate-Gaussian evidence p(w|z) (paper eq. 3.24). The model search
/// over `z` finds the configuration(s) that best explain the waveform.
///
/// This implementation is phased:
///  - Phase 1 (default, `enable_stochastic = false`): deterministic Fast
///    Bayesian Matching Pursuit (FBMP). Greedy forward selection over an
///    ROI-restricted candidate grid using rank-updated Gaussian evidence,
///    producing the MAP configuration and posterior-mean charges.
///  - Phase 2 (`enable_stochastic = true`): adds a Metropolis-Hastings-within-
///    Gibbs sampler over `z` and the light-curve time t0, giving the unbiased
///    t0 / mu estimators of paper eqs. 3.25-3.26. (Stub for now.)
///
/// A fast, non-iterative preconditioner reuses the upstream WaveformPrep
/// results (total charge -> mu0; threshold crossings -> candidate ROIs); no
/// LucyDDM-style iterative deconvolution is performed.
///
/// Shares the dictionary / ROI machinery design with WaveformAnalysisRAVEN.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisFSMP__
#define __RAT_WaveformAnalysisFSMP__

#include <TMatrixDfwd.h>
#include <TObject.h>
#include <TVectorDfwd.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <utility>
#include <vector>

namespace RAT {

class WaveformAnalysisFSMP : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisFSMP() : WaveformAnalysisFSMP("FSMP"){};

  WaveformAnalysisFSMP(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisFSMP", config_name) {
    Configure(config_name);
  };

  virtual ~WaveformAnalysisFSMP(){};

  void Configure(const std::string &config_name) override;

  void SetD(std::string param, double value) override;
  void SetI(std::string param, int value) override;

  /// Build the dictionary matrix fW of time-shifted SER templates.
  /// Rows index waveform samples, columns index candidate PE times on the
  /// upsampled grid (column j -> time j * digitizer_period / upsample_factor).
  void BuildDictionaryMatrix(int nsamples, double digitizer_period);

 protected:
  DBLinkPtr fDigit;

  // --- ROI / preconditioner ---
  bool process_threshold_crossing;  ///< Restrict analysis to threshold-crossing regions
  double voltage_threshold;         ///< Voltage threshold for ROI detection (mV)
  int threshold_region_padding;     ///< Samples to pad around ROIs

  // --- Single-PE response (lognormal SER) ---
  double lognormal_scale;  ///< LogNormal 'm' parameter
  double lognormal_shape;  ///< LogNormal 'sigma' parameter
  double vpe_charge;       ///< Nominal charge of a single PE (pC)

  // --- Dictionary ---
  TMatrixD fW;             ///< Dictionary matrix (nsamples x dict_size), in mV per unit charge weight
  double upsample_factor;  ///< Sub-sample resolution factor for candidate grid

  // --- Noise / charge prior (Bayesian evidence) ---
  double noise_sigma;  ///< Gaussian white-noise sigma (mV). <=0 means read from DIGITIZER.
  double gamma_k;      ///< Shape of gamma charge prior (paper: 1/0.4^2)
  double gamma_theta;  ///< Scale of gamma charge prior (paper: 0.4^2)

  // --- Model search ---
  size_t max_iterations;   ///< Max PEs added per ROI in greedy selection
  bool enable_stochastic;  ///< Phase 2: run the MCMC sampler after FBMP
  size_t n_mcmc_samples;   ///< Number of MCMC samples (Phase 2)
  size_t burn_in;          ///< Burn-in samples discarded (Phase 2)
  int random_seed;         ///< Seed for the sampler RNG (Phase 2)

  // --- NPE estimation ---
  bool npe_estimate;                 ///< Split posterior charge into integer PEs
  double npe_estimate_charge_width;  ///< Width of gaussian single-PE charge PDF
  size_t npe_estimate_max_pes;       ///< Upper bound on PEs per resolved hit

  // --- Dictionary cache ---
  bool dictionary_built;
  int cached_nsamples;
  double cached_digitizer_period;

  void DoAnalysis(DS::DigitPMT *digitpmt, const std::vector<UShort_t> &digitWfm) override;

  /// Find threshold crossing regions in a (negative-going) voltage waveform.
  std::vector<std::pair<int, int>> FindThresholdRegions(const std::vector<double> &voltWfm, double threshold,
                                                        int region_padding);

  /// Run FBMP (greedy forward selection) on one ROI and write PEs to fit_result.
  /// `logodds` is the per-PE log prior-odds (sparsity penalty) from the
  /// preconditioner occupancy estimate.
  void ProcessRegion(const std::vector<double> &voltWfm, int start_sample, int end_sample,
                     DS::WaveformAnalysisResult *fit_result, double gain_calibration, double logodds);

  /// Log Gaussian evidence log p(w|z) for the active set, returning the
  /// posterior-mean charges of the active columns in `charges_out`.
  /// `W_active` is (D x |P|), `voltVec` is (D). Uses the prior charge variance
  /// (gamma_k * gamma_theta^2) and noise variance (noise_sigma^2).
  double LogEvidence(const TMatrixD &W_active, const TVectorD &voltVec, TVectorD &charges_out);
};

}  // namespace RAT

#endif
