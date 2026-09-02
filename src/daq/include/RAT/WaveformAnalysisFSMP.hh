////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisFSMP
///
/// \brief Reconstruct photoelectron times and charges with Fast Stochastic
/// Matching Pursuit (FSMP)
///
/// \author Ravi Carpen Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     24 Jun 2026: Initial commit
///     31 Jul 2026: Greedy search split out into WaveformAnalysisGreedyMP
///
/// \details
/// FSMP is the Bayesian waveform analysis of Xu et al. 2022, JINST 17 P06040
/// (arXiv:2112.06913). The waveform is modelled as a sparse spike train of PEs
/// convolved with a single PE response (SER) template plus Gaussian white noise.
/// Given the evidence p(w|z) of a configuration z, the per PE charges integrated
/// out analytically, configurations are explored with a
/// Metropolis-Hastings-within-Gibbs sampler (birth/death/shift moves) run
/// jointly with the light curve time t0 (paper eq. 2.2). The paper describes
/// this as replacing the greedy search of FBMP with stochastic sampling; that
/// greedy search is WaveformAnalysisGreedyMP, and pointing seed_analyzer at it
/// gives the chain its starting configuration.
///
/// Because the sampler carries t0 and the intensity mu, FSMP estimates the
/// incident light directly rather than only the individual PEs. Alongside the
/// MAP configuration's PE times and charges it reports "fsmp_t0", "fsmp_mu" and
/// "fsmp_npe" (paper eqs. 3.25-3.26), computed once per waveform across all
/// regions jointly.
///
/// The MAP configuration also defines an expected waveform, the model the fit
/// settled on. Setting "store_expected_waveform" attaches it to the
/// WaveformAnalysisResult, where outntuple picks it up alongside the observed
/// waveform so the two can be overlaid.
///
/// Template types supported:
/// - Lognormal
/// - Gaussian
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisFSMP__
#define __RAT_WaveformAnalysisFSMP__

#include <TMatrixD.h>
#include <TRandom3.h>
#include <TVectorD.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/DS/WaveformAnalysisResult.hh>
#include <RAT/SERDictionary.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <map>
#include <string>
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
  void SetS(std::string param, std::string value) override;

  void BeginOfRun(DS::Run *run) override;

 protected:
  /// A threshold crossing region and the PE configuration assigned to it
  struct Region {
    int start_sample = 0;     ///< First waveform sample in the region
    int end_sample = 0;       ///< Last waveform sample in the region
    int dict_start = 0;       ///< Global dictionary column of local column 0
    int dict_cols = 0;        ///< Number of candidate columns in the region
    TMatrixD W;               ///< Region sub-dictionary (region_length x dict_cols)
    TVectorD v;               ///< Region waveform (region_length)
    std::vector<int> active;  ///< Occupied local columns, sorted ascending
    TVectorD charges;         ///< Posterior-mean charges, aligned with `active`
    double logev = 0.0;       ///< log p(w|z) for the current `active`
  };

  DBLinkPtr fDigit;

  bool process_threshold_crossing;  ///< Whether to use threshold crossing region processing
  double voltage_threshold;         ///< Voltage threshold for threshold crossing region detection
  int threshold_region_padding;     ///< Number of samples to pad around threshold crossing regions

  /// SER templates and per-PE charge scales, shared with WaveformAnalysisGreedyMP
  /// so a seed and the sampler that starts from it describe the same detector.
  SERDictionary fDict;

  // Resolved from fDict for the PMT currently being analyzed, at the top of
  // DoAnalysis(), so the kernels below can stay per-waveform scalars.
  double vpe_charge;   ///< Nominal charge of single PE in pC
  double gamma_k;      ///< Shape of the per-PE charge prior
  double gamma_theta;  ///< Scale of the per-PE charge prior

  // Algorithm configuration
  double upsample_factor;  ///< Dictionary upsampling factor for sub-sample resolution
  size_t max_iterations;   ///< Maximum PEs taken from the seed per region

  // Bayesian evidence parameters
  double noise_sigma;  ///< Gaussian white-noise sigma of the waveform in mV. Must be > 0.

  // Initial configuration
  std::string seed_analyzer;  ///< Analyzer whose result starts the chain, empty to start from no PEs
  bool seed_missing_warned;   ///< Limits the "no seed result" warning to once per run

  // Stochastic sampler
  size_t n_mcmc_samples;  ///< Number of post-burn-in MCMC samples
  size_t burn_in;         ///< Burn-in samples discarded before recording
  TRandom3 fRNG;          ///< Sampler RNG, seeded from the global engine at BeginOfRun

  // Light curve prior on PE arrival times (paper eq. 2.2)
  double lightcurve_tau;    ///< Exponential time constant tau_l (ns), 0 for pure Gaussian
  double lightcurve_sigma;  ///< Timing spread sigma_l (ns), mainly PMT transit time spread
  double
      lightcurve_floor;  ///< Fraction of the prior spread flat across the window, for PEs the curve does not describe
  double lightcurve_window;  ///< Digitizer window (ns) the floor is spread over, set per waveform
  double t0_step;            ///< Random-walk proposal step for t0 (ns)

  // NPE estimation parameters
  bool npe_estimate;                 ///< Whether to perform NPE estimation on resolved wave packets
  double npe_estimate_charge_width;  ///< Width of Gaussian single-PE charge distribution
  size_t npe_estimate_max_pes;       ///< Upper limit for NPE estimation
  double weight_merge_window;        ///< Time window (ns) for merging nearby weights, 0 to disable

  // Expected waveform output
  bool store_expected_waveform;  ///< Whether to hand the MAP model waveform to the WaveformAnalysisResult

  // Dictionary management
  int cached_nsamples;             ///< Cached number of samples for dictionary
  double cached_digitizer_period;  ///< Cached digitizer period for dictionary

  void DoAnalysis(DS::DigitPMT *digitpmt, const std::vector<UShort_t> &digitWfm) override;

  /// Gather the columns `cols` of `W` into a (nrows x |cols|) matrix
  static TMatrixD BuildActive(const TMatrixD &W, int nrows, const std::vector<int> &cols);

  /// Find threshold crossing regions in waveform for efficient processing
  std::vector<std::pair<int, int>> FindThresholdRegions(const std::vector<double> &voltWfm, double threshold,
                                                        int region_padding);

  /// Slice the dictionary and waveform into a region, false if it holds no columns
  bool PrepareRegion(const TMatrixD &fW, const std::vector<double> &voltWfm, int start_sample, int end_sample,
                     Region &region_out);

  /// Write one region's resolved atoms to fit_result, tagged with `extra_foms`
  void EmitRegion(const Region &region, DS::WaveformAnalysisResult *fit_result, double gain_calibration, double chi2ndf,
                  const std::map<std::string, double> &extra_foms);

  /// Fold every region's resolved atoms back through the full-window dictionary
  /// `fW`, giving the model waveform in mV over all `nsamples` digitizer samples
  std::vector<double> ExpectedWaveform(const std::vector<Region> &regions, const TMatrixD &fW, int nsamples) const;

  /// Log evidence log p(w|z) of the active set, with its posterior-mean charges
  double LogEvidence(const TMatrixD &W_active, const TVectorD &voltVec, TVectorD &charges_out);

  /// Seed the regions from seed_analyzer's result, returning the PEs seeded
  size_t SeedRegions(DS::DigitPMT *digitpmt, std::vector<Region> &regions);

  /// Sample configurations z and the light curve time t0 jointly over every
  /// region, overwriting `regions` with the MAP one and returning the estimators
  /// of paper eqs. 3.25-3.26. The occupancy prior spans all `dict_total` columns.
  void SampleConfigurations(std::vector<Region> &regions, int dict_total, double mu0, double &t0_hat, double &mu_hat,
                            double &npe_hat);

  /// Importance-reweighted intensity MLE (paper eqs. 3.25-3.26) from the PE-count
  /// histogram `n_hist` drawn under `mu_ref`, given the in-window light `mass_scale`
  double IntensityMLE(const std::vector<size_t> &n_hist, double mu_ref, double mass_scale) const;

  /// Normalized light-curve density phi(dt) (paper eq. 2.2), dt = t - t0
  double LightCurve(double dt) const;
};

}  // namespace RAT

#endif
