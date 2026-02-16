////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisRSNNLS
///
/// \brief Perform reverse sparse non-negative least squares fitting on digitized waveforms
///
/// \author Ravi Carpen Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     12 Sep 2025: Initial commit
///     15 Sep 2025: Add Gaussian template option
///
/// \details
/// This class performs reverse sparse non-negative least squares (rsNNLS) analysis
/// on digitized PMT waveforms to reconstruct photoelectron times and charges.
///
/// The algorithm uses region-based processing for improved efficiency:
/// 1. Builds a dictionary matrix of time-shifted templates
/// 2. Identifies threshold crossing regions in the waveform for localized processing
/// 3. For each region, extracts relevant dictionary submatrix and applies NNLS fitting
/// 4. Uses iterative thresholding to remove low-weight components and redistribute weights
/// 5. Extracts PE times and charges from remaining significant weights
///
/// Template types supported:
/// - Lognormal
/// - Gaussian
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisRSNNLS__
#define __RAT_WaveformAnalysisRSNNLS__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <utility>
#include <vector>

#include "TMatrixD.h"

namespace RAT {

class WaveformAnalysisRSNNLS : public WaveformAnalyzerBase {
 public:
  WaveformAnalysisRSNNLS() : WaveformAnalysisRSNNLS("rsNNLS"){};

  WaveformAnalysisRSNNLS(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisRSNNLS", config_name) {
    Configure(config_name);
  };

  virtual ~WaveformAnalysisRSNNLS(){};

  void BuildDictionaryMatrix(int nsamples, double digitizer_period);

  void Configure(const std::string &config_name) override;

  void SetD(std::string param, double value) override;
  void SetI(std::string param, int value) override;

 protected:
  DBLinkPtr fDigit;

  bool process_threshold_crossing;  ///< Whether to use threshold crossing region processing
  double voltage_threshold;         ///< Voltage threshold for threshold crossing region detection
  int threshold_region_padding;     ///< Number of samples to pad around threshold crossing regions

  int template_type;  ///< Template type: 0=lognormal, 1=gaussian

  // LogNormal template parameters
  double lognormal_scale;  ///< LogNormal 'm' parameter for SPE template
  double lognormal_shape;  ///< LogNormal 'sigma' parameter for SPE template

  // Gaussian template parameters
  double gaussian_width;  ///< Gaussian 'sigma' parameter for SPE template

  double vpe_charge;  ///< Nominal charge of single PE in pC

  // Algorithm configuration
  TMatrixD fW;             ///< Dictionary matrix for NNLS (nsamples × dict_size)
  double epsilon;          ///< NNLS convergence tolerance
  size_t max_iterations;   ///< Maximum iterations for iterative thresholding
  double upsample_factor;  ///< Dictionary upsampling factor for sub-sample resolution

  // Thresholding parameters
  double weight_threshold;     ///< Minimum weight threshold for component significance
  double weight_merge_window;  ///< Time window (ns) for merging nearby weights before NPE estimation

  // NPE estimation parameters
  bool npe_estimate;                 ///< Whether to perform NPE estimation on resolved wave packets
  double npe_estimate_charge_width;  ///< Width of Gaussian single-PE charge distribution
  size_t npe_estimate_max_pes;       ///< Upper limit for NPE estimation

  // Dictionary management
  bool dictionary_built;           ///< Flag to track if dictionary has been built
  int cached_nsamples;             ///< Cached number of samples for dictionary
  double cached_digitizer_period;  ///< Cached digitizer period for dictionary

  void DoAnalysis(DS::DigitPMT *digitpmt, const std::vector<UShort_t> &digitWfm) override;

  /// Perform reverse sparse NNLS with iterative thresholding on a region submatrix
  TVectorD Thresholded_rsNNLS(const TMatrixD &W_region, const TVectorD &voltVec, const double threshold,
                              double &chi2ndf_out, int &iterations_out);

  /// Find threshold crossing regions in waveform for efficient processing
  std::vector<std::pair<int, int>> FindThresholdRegions(const std::vector<double> &voltWfm, double threshold,
                                                        int region_padding);

  /// Process a single threshold crossing region with rsNNLS
  void ProcessThresholdRegion(const std::vector<double> &voltWfm, int start_sample, int end_sample,
                              DS::WaveformAnalysisResult *fit_result, double gain_calibration);

  /// Extract photoelectrons from significant weights in the region
  void ExtractPhotoelectrons(const TVectorD &region_weights, int dict_start, int dict_cols, int start_sample,
                             int end_sample, double chi2ndf, int iterations_ran, DS::WaveformAnalysisResult *fit_result,
                             double gain_calibration);

  /// Merge nearby weights within a time window to prevent PE overcounting
  /// Returns vector of (time, merged_weight) pairs
  std::vector<std::pair<double, double>> MergeNearbyWeights(const TVectorD &region_weights, int dict_start,
                                                            int dict_cols, double merge_window);
};

}  // namespace RAT

#endif
