////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisGreedyMP
///
/// \brief Reconstruct photoelectron times and charges by greedy Bayesian
/// matching pursuit
///
/// \author Ravi Carpen Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     31 Jul 2026: Split out of WaveformAnalysisFSMP
///
/// \details
/// Models the waveform as a sparse spike train of PEs convolved with a single PE
/// response (SER) template plus Gaussian white noise, and infers which points of
/// a fine time grid host a PE. This is the greedy search of fast Bayesian
/// matching pursuit (Schniter, Potter and Ziniel, ITA Workshop 2008), the
/// starting point Xu et al. 2022 (JINST 17 P06040) take for FSMP.
///
/// Each threshold crossing region is searched independently, an atom at a time:
/// 1. Correlate every unoccupied dictionary column with the fit residual
/// 2. Shortlist the greedy_shortlist best-correlated columns
/// 3. Score those with the full evidence p(w|z), the per PE charges integrated
///    out analytically, and keep whichever most improves it net of a per-PE
///    sparsity penalty
/// 4. Refit and repeat until no column improves the posterior
///
/// Template types supported:
/// - Lognormal
/// - Gaussian
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisGreedyMP__
#define __RAT_WaveformAnalysisGreedyMP__

#include <TMatrixD.h>
#include <TVectorD.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/DS/WaveformAnalysisResult.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace RAT {

class WaveformAnalysisGreedyMP : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisGreedyMP() : WaveformAnalysisGreedyMP("GreedyMP"){};

  WaveformAnalysisGreedyMP(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisGreedyMP", config_name) {
    Configure(config_name);
  };

  virtual ~WaveformAnalysisGreedyMP(){};

  void BuildDictionaryMatrix(int nsamples, double digitizer_period, double width, TMatrixD &W_out);

  void Configure(const std::string &config_name) override;

  void SetD(std::string param, double value) override;
  void SetI(std::string param, int value) override;
  void SetS(std::string param, std::string value) override;

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

  int template_type;  ///< Template type: 0=lognormal, 1=gaussian

  // LogNormal template parameters
  double lognormal_scale;  ///< LogNormal 'm' parameter for SPE template
  double lognormal_shape;  ///< LogNormal 'sigma' parameter for SPE template

  // Gaussian template parameters
  double gaussian_width;                      ///< Gaussian 'sigma' parameter for SPE template
  std::vector<int> gaussian_width_types;      ///< PMT types with their own template width
  std::vector<double> gaussian_width_values;  ///< Widths of those types; others use gaussian_width

  double vpe_charge;  ///< Nominal charge of single PE in pC

  // Algorithm configuration
  std::map<int, TMatrixD> fWCache;  ///< Dictionary per template, keyed by width in ps (-1 = lognormal)
  double upsample_factor;           ///< Dictionary upsampling factor for sub-sample resolution
  size_t max_iterations;            ///< Maximum PEs placed per region
  int greedy_shortlist;             ///< Best-correlated columns scored per greedy step

  // Bayesian evidence parameters
  double noise_sigma;  ///< Gaussian white-noise sigma of the waveform in mV. Must be > 0.
  double gamma_k;      ///< Shape of the per-PE charge prior
  double gamma_theta;  ///< Scale of the per-PE charge prior

  // Initial configuration
  std::string seed_analyzer;  ///< Analyzer whose result seeds the search, empty to search from scratch
  bool seed_missing_warned;   ///< Limits the "no seed result" warning to once per run

  // NPE estimation parameters
  bool npe_estimate;                 ///< Whether to perform NPE estimation on resolved wave packets
  double npe_estimate_charge_width;  ///< Width of Gaussian single-PE charge distribution
  size_t npe_estimate_max_pes;       ///< Upper limit for NPE estimation
  double weight_merge_window;        ///< Time window (ns) for merging nearby weights, 0 to disable

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

  /// Write one region's resolved atoms to fit_result, with the NPE split applied
  void EmitRegion(const Region &region, DS::WaveformAnalysisResult *fit_result, double gain_calibration,
                  double chi2ndf);

  /// Log evidence log p(w|z) of the active set, with its posterior-mean charges
  double LogEvidence(const TMatrixD &W_active, const TVectorD &voltVec, TVectorD &charges_out);

  /// Seed the regions from seed_analyzer's result, returning the PEs seeded
  size_t SeedRegions(DS::DigitPMT *digitpmt, std::vector<Region> &regions);

  /// Greedily select PE columns, maximising the evidence net of `logodds` per PE
  void GreedySelect(Region &region, double logodds);
};

}  // namespace RAT

#endif
