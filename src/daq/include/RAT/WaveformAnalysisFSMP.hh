////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisFSMP
///
/// \brief Reconstruct photoelectron times and charges with Fast Stochastic
/// Matching Pursuit (FSMP).
///
/// \author Ravi Carpen Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     24 Jun 2026: Initial commit
///
/// \details
/// Implements the Fast Stochastic Matching Pursuit method of Xu et al. 2022
/// (JINST 17 P06040, arXiv:2112.06913).
///
/// The waveform is modelled as a sparse spike train of PEs convolved with a
/// single-PE response (SER) template plus Gaussian white noise (paper eq. 2.5).
/// A dictionary of time-shifted SER templates on an upsampled grid spans the
/// candidate PE times. For a given PE configuration z (which grid points host a
/// PE), the per-PE charges are integrated out analytically, yielding a
/// multivariate-Gaussian evidence p(w|z) (paper eq. 3.24). Candidate times are
/// restricted to threshold-crossing regions of interest, and a non-iterative
/// preconditioner derived from the upstream WaveformPrep total charge sets the
/// occupancy prior.
///
/// The configuration z is found by a Metropolis-Hastings-within-Gibbs sampler
/// over z (birth/death/shift moves) and the light-curve time t0, initialised
/// from a greedy forward-selection solution. The sampler yields the unbiased t0
/// and intensity (mu) estimators of paper eqs. 3.25-3.26, stored as figures of
/// merit alongside the per-PE times and charges. Setting enable_stochastic
/// false skips the sampler and reports the greedy solution only.
///
/// The dictionary and region-of-interest machinery mirror WaveformAnalysisRAVEN.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisFSMP__
#define __RAT_WaveformAnalysisFSMP__

#include <TMatrixDfwd.h>
#include <TObject.h>
#include <TRandom3.h>
#include <TVectorDfwd.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <memory>
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
  size_t max_iterations;   ///< Max PEs selected per ROI in greedy initialisation
  bool enable_stochastic;  ///< Run the MCMC sampler; if false, report the greedy solution only
  size_t n_mcmc_samples;   ///< Number of post-burn-in MCMC samples
  size_t burn_in;          ///< Burn-in samples discarded before recording
  int random_seed;         ///< Seed for the sampler RNG

  // --- Light curve prior on PE arrival times (paper eq. 2.2) ---
  double lightcurve_tau;    ///< Exponential time constant tau_l (ns). 0 => pure Gaussian (Cherenkov).
  double lightcurve_sigma;  ///< Timing spread sigma_l (ns), mainly PMT TTS.
  double t0_step;           ///< Random-walk proposal step for t0 (ns).

  std::unique_ptr<TRandom3> fRNG;  ///< RNG for the stochastic sampler.

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

  /// Analyze one ROI and write reconstructed PEs to fit_result. Runs the greedy
  /// initialisation, then (if enable_stochastic) the MCMC sampler.
  /// `logodds` is the flat per-PE log prior-odds used by the greedy step;
  /// `mu0` is the preconditioner intensity used by the sampler's occupancy prior.
  void ProcessRegion(const std::vector<double> &voltWfm, int start_sample, int end_sample,
                     DS::WaveformAnalysisResult *fit_result, double gain_calibration, double logodds, double mu0);

  /// Log Gaussian evidence log p(w|z) for the active set, returning the
  /// posterior-mean charges of the active columns in `charges_out`.
  /// `W_active` is (D x |P|), `voltVec` is (D). Uses the prior charge variance
  /// (gamma_k * gamma_theta^2) and noise variance (noise_sigma^2).
  double LogEvidence(const TMatrixD &W_active, const TVectorD &voltVec, TVectorD &charges_out);

  /// Greedy forward selection of PE columns within a region sub-dictionary
  /// `W_roi` (region_length x dict_cols), maximising the log evidence plus the
  /// flat occupancy penalty `logodds`. Fills `active` (local column indices) and
  /// `charges` (posterior-mean charges of those columns).
  void GreedySelect(const TMatrixD &W_roi, const TVectorD &v, int dict_cols, double logodds, std::vector<int> &active,
                    TVectorD &charges);

  /// Metropolis-Hastings-within-Gibbs sampler over the PE configuration z
  /// (birth/death/shift moves) and the light-curve time t0. Initialised from
  /// `active`/`charges`, it overwrites them with the MAP configuration and
  /// returns the posterior-mean estimators t0_hat, mu_hat (paper eqs.
  /// 3.25-3.26). `dict_start` maps local columns to global time.
  void SampleConfigurations(const TMatrixD &W_roi, const TVectorD &v, int dict_cols, int dict_start, double mu0,
                            std::vector<int> &active, TVectorD &charges, double &t0_hat, double &mu_hat);

  /// Normalized light-curve density phi(dt) (paper eq. 2.2), dt = t - t0.
  /// Ex-Gaussian for tau_l>0, pure Gaussian for tau_l->0.
  double LightCurve(double dt) const;
};

}  // namespace RAT

#endif
