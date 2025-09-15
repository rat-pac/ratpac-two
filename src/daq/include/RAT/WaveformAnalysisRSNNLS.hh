////////////////////////////////////////////////////////////////////
/// \class RAT::WaveformAnalysisRSNNLS
///
/// \brief Perform reverse sparse non-negative least squares fitting on digitized waveforms
///
/// \author Ravi Pitelka <rpitelka@sas.upenn.edu>
///
/// REVISION HISTORY:\n
///     21 Aug 2025: Initial commit
///     10 Sep 2025: Parameter optimization and digitizer integration
///
/// \details
/// This class performs reverse sparse non-negative least squares (rsNNLS) analysis
/// on digitized PMT waveforms to reconstruct photoelectron times and charges.
///
/// The algorithm uses region-based processing for improved efficiency:
/// 1. Builds a dictionary matrix of time-shifted templates at configuration time
/// 2. Identifies threshold crossing regions in the waveform for localized processing
/// 3. For each region, extracts relevant dictionary submatrix and applies NNLS fitting
/// 4. Uses iterative thresholding to remove low-weight components and redistribute weights
/// 5. Extracts PE times and charges from remaining significant weights
///
/// Template types supported:
/// - LogNormal: Traditional single photoelectron waveform shape
/// - Gaussian: Simplified symmetric template for faster processing
///
/// Dictionary matrix construction uses digitizer parameters from DIGITIZER.ratdb:
/// - Sampling rate and number of samples define the time grid
/// - Upsampling factor provides sub-sample time resolution
/// - Template parameters (LogNormal or Gaussian) model single photoelectron waveforms
///
/// Region-based processing improves performance by only analyzing signal-containing regions
/// and reduces edge artifacts by using appropriate dictionary template selection.
////////////////////////////////////////////////////////////////////
#ifndef __RAT_WaveformAnalysisRSNNLS__
#define __RAT_WaveformAnalysisRSNNLS__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <RAT/WaveformAnalyzerBase.hh>
#include <vector>

#include "TMatrixD.h"

namespace RAT {

class WaveformAnalysisRSNNLS : public WaveformAnalyzerBase {
 public:
  /// Default constructor using "rsNNLS" configuration
  WaveformAnalysisRSNNLS() : WaveformAnalysisRSNNLS("rsNNLS"){};

  /// Constructor with custom configuration name
  /// @param config_name Configuration index for DIGITIZER_ANALYSIS table
  WaveformAnalysisRSNNLS(std::string config_name) : WaveformAnalyzerBase("WaveformAnalysisRSNNLS", config_name) {
    Configure(config_name);
  };

  virtual ~WaveformAnalysisRSNNLS(){};

  /// Build the dictionary matrix for NNLS fitting using digitizer parameters
  /// @param nsamples Number of samples in waveform (from digitizer config)
  /// @param digitizer_period Sampling period in ns (from digitizer config)
  void BuildDictionaryMatrix(int nsamples, double digitizer_period);

  /// Configure the processor using DIGITIZER_ANALYSIS parameters
  /// @param config_name Configuration index for parameter lookup
  void Configure(const std::string &config_name) override;

  /// Override parameter setting functions for runtime configuration
  void SetD(std::string param, double value) override;
  void SetI(std::string param, int value) override;

 protected:
  // Database and configuration
  DBLinkPtr fDigit;  ///< Link to DIGITIZER_ANALYSIS database table

  // Single photoelectron waveform parameters
  int template_type;  ///< Template type: 0=lognormal, 1=gaussian

  // LogNormal template parameters
  double lognormal_scale;  ///< LogNormal 'm' parameter for SPE template
  double lognormal_shape;  ///< LogNormal 'sigma' parameter for SPE template

  // Gaussian template parameters
  double gaussian_width;  ///< Gaussian sigma parameter for SPE template

  double vpe_charge;  ///< Nominal charge of single PE in pC

  // Algorithm configuration
  TMatrixD fW;             ///< Dictionary matrix for NNLS (nsamples × dict_size)
  double epsilon;          ///< NNLS convergence tolerance
  size_t max_iterations;   ///< Maximum iterations for iterative thresholding
  double upsample_factor;  ///< Dictionary upsampling factor for sub-sample resolution

  // Thresholding parameters
  double weight_threshold;   ///< Minimum weight threshold for component significance
  double voltage_threshold;  ///< Voltage threshold for threshold crossing region detection

  // Analysis results (computed per waveform)
  double chi2ndf;      ///< Chi-squared per degree of freedom for goodness of fit
  int iterations_ran;  ///< Number of thresholding iterations performed

  // Dictionary management
  bool dictionary_built;           ///< Flag to track if dictionary has been built
  int cached_nsamples;             ///< Cached number of samples for dictionary
  double cached_digitizer_period;  ///< Cached digitizer period for dictionary

  /// Main analysis method called for each PMT waveform
  /// @param digitpmt Pointer to DigitPMT object for storing results
  /// @param digitWfm Digital waveform in ADC units
  void DoAnalysis(DS::DigitPMT *digitpmt, const std::vector<UShort_t> &digitWfm) override;

  /// Perform reverse sparse NNLS with iterative thresholding on a region submatrix
  /// @param W_region Dictionary submatrix for the region
  /// @param voltVec Input voltage vector for the region
  /// @param threshold Minimum weight threshold for component significance
  /// @return Weight vector with components above threshold
  TVectorD Thresholded_rsNNLS(const TMatrixD &W_region, const TVectorD &voltVec, const double threshold);

  /// Find threshold crossing regions in waveform for efficient processing
  /// @param voltWfm Input voltage waveform (pedestal-subtracted)
  /// @param threshold Voltage threshold for signal detection
  /// @return Vector of (start_sample, end_sample) pairs for regions above threshold
  std::vector<std::pair<int, int>> FindThresholdRegions(const std::vector<double> &voltWfm, double threshold);

  /// Process a single threshold crossing region with rsNNLS
  /// @param voltWfm Full voltage waveform
  /// @param start_sample Start sample of region
  /// @param end_sample End sample of region
  /// @param fit_result Result object to store extracted PEs
  void ProcessThresholdRegion(const std::vector<double> &voltWfm, int start_sample, int end_sample,
                              DS::WaveformAnalysisResult *fit_result);
};

}  // namespace RAT

#endif
