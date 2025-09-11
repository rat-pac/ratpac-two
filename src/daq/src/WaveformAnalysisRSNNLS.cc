#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisRSNNLS.hh>
#include <limits>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/NNLS.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

void WaveformAnalysisRSNNLS::Configure(const std::string& config_name) {
  // Load analysis parameters from DIGITIZER_ANALYSIS database
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);

    // Single photoelectron waveform parameters
    vpe_scale = fDigit->GetD("vpe_scale");    // LogNormal 'm' parameter
    vpe_shape = fDigit->GetD("vpe_shape");    // LogNormal 'sigma' parameter
    vpe_charge = fDigit->GetD("vpe_charge");  // Nominal PE charge in pC

    // Algorithm configuration
    max_iterations = fDigit->GetI("max_iterations");        // Max thresholding iterations
    weight_threshold = fDigit->GetD("weight_threshold");    // Component significance threshold
    upsample_factor = fDigit->GetD("upsampling_factor");    // Dictionary upsampling factor
    epsilon = fDigit->GetD("nnls_tolerance");               // NNLS convergence tolerance
    voltage_threshold = fDigit->GetD("voltage_threshold");  // Voltage threshold for region detection
    digitizer_name = fDigit->GetS("digitizer_name");        // Digitizer configuration name

    info << "WaveformAnalysisRSNNLS: Read digitizer_name: '" << digitizer_name << "'" << newline;

    // Validate critical parameters
    if (upsample_factor <= 0) {
      RAT::Log::Die("WaveformAnalysisRSNNLS: Invalid upsampling factor.");
    }

  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Unable to find analysis parameters.");
  }

  // Build dictionary matrix using digitizer hardware configuration
  try {
    info << "WaveformAnalysisRSNNLS: Attempting to load digitizer config: " << digitizer_name << newline;
    DBLinkPtr digitizer_db = DB::Get()->GetLink("DIGITIZER", digitizer_name);

    info << "WaveformAnalysisRSNNLS: Successfully loaded digitizer config" << newline;
    double sampling_rate = digitizer_db->GetD("sampling_rate");  // Sampling rate in GHz
    int nsamples = digitizer_db->GetD("nsamples");  // Number of samples per trace (using GetD like Digitizer.cc)
    digitizer_period = 1.0 / sampling_rate;         // Convert GHz to ns sampling period

    BuildDictionaryMatrix(nsamples, digitizer_period);

  } catch (DBNotFoundError&) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Unable to find digitizer configuration: " + digitizer_name);
  }
}

void WaveformAnalysisRSNNLS::BuildDictionaryMatrix(int nsamples, double digitizer_period) {
  /**
   * Build dictionary matrix for NNLS fitting using time-shifted LogNormal templates.
   *
   * Dictionary structure:
   * - Rows: Time samples (nsamples)
   * - Columns: Time-shifted templates (nsamples * upsample_factor)
   * - Each column represents a LogNormal template with different delay
   * - Templates are negative for deconvolution (minimize ||Wx + y||²)
   *
   * Time grid with peak offset correction:
   * - Sample i corresponds to time: i * digitizer_period
   * - Dictionary column j has delay: j * digitizer_period / upsample_factor
   * - PE time extracted as: delay + peak_offset (where peak_offset accounts for LogNormal peak position)
   */

  const int dict_size = static_cast<int>(nsamples * upsample_factor);
  fW.ResizeTo(nsamples, dict_size);
  fW.Zero();

  // Generate dictionary with time-shifted LogNormal templates
  for (int col = 0; col < dict_size; ++col) {
    double delay = col * digitizer_period / upsample_factor;

    // Use simple delay as LogNormal shift parameter
    // Peak offset will be handled during time extraction
    double lognormal_shift = delay;

    // Fill column with negative LogNormal template (negative for deconvolution)
    for (int row = 0; row < nsamples; ++row) {
      double sample_time = row * digitizer_period;
      double template_val = 0.0;

      // LogNormal is only defined for sample_time > lognormal_shift
      if (sample_time > lognormal_shift) {
        template_val = TMath::LogNormal(sample_time, vpe_shape, lognormal_shift, vpe_scale);
      }

      fW(row, col) = -template_val;  // Negative for deconvolution formulation
    }
  }
}

void WaveformAnalysisRSNNLS::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  // Validate pedestal has been computed
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Pedestal is invalid! Did you run WaveformPrep first?");
  }

  // Verify waveform size matches pre-built dictionary matrix
  if (static_cast<int>(digitWfm.size()) != fW.GetNrows()) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Waveform size mismatch with dictionary matrix.");
  }

  // Convert waveform from ADC units to voltage (mV, pedestal-subtracted)
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  // Find threshold crossing regions for efficient processing
  std::vector<std::pair<int, int>> crossing_regions = FindThresholdRegions(voltWfm, voltage_threshold);

  if (crossing_regions.empty()) {
    // No signal above threshold - return empty result
    digitpmt->GetOrCreateWaveformAnalysisResult("rsNNLS");
    return;
  }

  // Process each threshold crossing region independently
  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("rsNNLS");

  for (const auto& region : crossing_regions) {
    int start_sample = region.first;
    int end_sample = region.second;

    // Perform NNLS on this region with appropriate time mapping
    ProcessThresholdRegion(voltWfm, start_sample, end_sample, fit_result);
  }
}

TVectorD WaveformAnalysisRSNNLS::Thresholded_rsNNLS(const std::vector<double>& voltWfm, const double threshold) {
  /**
   * Reverse Sparse Non-Negative Least Squares with Iterative Thresholding
   *
   * Algorithm:
   * 1. Solve initial NNLS problem: minimize ||Wx + y||² subject to x ≥ 0
   * 2. Iteratively remove components with weights below threshold
   * 3. Re-solve NNLS on reduced active set to redistribute weights
   * 4. Continue until all remaining weights exceed threshold or convergence
   *
   * This approach allows weight redistribution and improves sparsity compared
   * to simple post-thresholding of the full NNLS solution.
   *
   * @param voltWfm Input voltage waveform (pedestal-subtracted, in mV)
   * @param threshold Minimum weight threshold for component significance
   * @return Weight vector with thresholded components
   */

  const int D = static_cast<int>(voltWfm.size());
  if (fW.GetNrows() != D) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Dictionary row dimension mismatch.");
  }
  const int K = fW.GetNcols();

  TVectorD x(D);
  for (int i = 0; i < D; ++i) x(i) = voltWfm[i];

  // Initial NNLS solve with configured tolerance
  TVectorD h_full(K);
  h_full.Zero();
  h_full = Math::NNLS_LawsonHanson(fW, x, epsilon, 0);

  // Build initial active set of positive components
  std::vector<int> P;
  P.reserve(K);
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) P.push_back(j);
  }

  // Helper lambda to extract dictionary submatrix for active components
  auto subCols = [](const TMatrixD& W, const std::vector<int>& cols) {
    TMatrixD S(W.GetNrows(), cols.size());
    for (size_t jj = 0; jj < cols.size(); ++jj) {
      int c = cols[jj];
      for (int i = 0; i < W.GetNrows(); ++i) S(i, jj) = W(i, c);
    }
    return S;
  };

  // Iterative thresholding: remove components below threshold and re-solve
  int iter = 0;
  const int max_iter = std::min(50, K);  // Prevent excessive iterations
  iterations_ran = 0;

  while (!P.empty() && iter++ < max_iter) {
    iterations_ran = iter;
    // Find component with minimum weight
    double minVal = std::numeric_limits<double>::infinity();
    size_t minPos = 0;
    for (size_t k = 0; k < P.size(); ++k) {
      double v = h_full(P[k]);
      if (v < minVal) {
        minVal = v;
        minPos = k;
      }
    }

    // Stop if minimum weight exceeds threshold
    if (minVal >= threshold) break;

    // Remove component with smallest weight
    h_full(P[minPos]) = 0.0;
    P.erase(P.begin() + minPos);
    if (P.empty()) {
      h_full.Zero();
      return h_full;
    }

    // Re-solve on reduced active set
    TMatrixD W_P = subCols(fW, P);
    TVectorD h_reduced(P.size());
    h_reduced.Zero();
    h_reduced = Math::NNLS_LawsonHanson(W_P, x, epsilon, 0);

    // Update full weight vector
    h_full.Zero();
    for (size_t k = 0; k < P.size(); ++k) {
      h_full(P[k]) = h_reduced(k);
    }
  }

  // Ensure numerical stability (guard against small negative values)
  for (int j = 0; j < K; ++j) {
    if (h_full(j) < 0.0) h_full(j) = 0.0;
  }

  // Calculate chi-squared goodness of fit for solution quality assessment
  TVectorD fitted = fW * h_full;  // Fitted waveform: W * h
  double chi2_sum = 0.0;
  for (int i = 0; i < D; ++i) {
    double residual = x(i) - fitted(i);  // Data - model residual
    chi2_sum += residual * residual;
  }

  // Compute degrees of freedom (data points - active parameters)
  int active_components = 0;
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) active_components++;
  }
  int dof = std::max(1, D - active_components);  // Prevent division by zero
  chi2ndf = chi2_sum / dof;

  return h_full;
}

TVectorD WaveformAnalysisRSNNLS::Thresholded_rsNNLS_Region(const TMatrixD& W_region, const TVectorD& voltVec,
                                                           const double threshold) {
  /**
   * Reverse Sparse Non-Negative Least Squares with Iterative Thresholding for region processing
   *
   * Same algorithm as Thresholded_rsNNLS but works on region submatrices for efficiency:
   * 1. Solve initial NNLS problem: minimize ||W_region * x + voltVec||² subject to x ≥ 0
   * 2. Iteratively remove components with weights below threshold
   * 3. Re-solve NNLS on reduced active set to redistribute weights
   * 4. Continue until all remaining weights exceed threshold or convergence
   *
   * @param W_region Dictionary submatrix for this region
   * @param voltVec Input voltage vector for this region
   * @param threshold Minimum weight threshold for component significance
   * @return Weight vector with thresholded components
   */

  const int D = voltVec.GetNrows();
  const int K = W_region.GetNcols();

  if (W_region.GetNrows() != D) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Dictionary region row dimension mismatch.");
  }

  // Initial NNLS solve with configured tolerance
  TVectorD h_full(K);
  h_full.Zero();
  h_full = Math::NNLS_LawsonHanson(W_region, voltVec, epsilon, 0);

  // Build initial active set of positive components
  std::vector<int> P;
  P.reserve(K);
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) P.push_back(j);
  }

  // Helper lambda to extract dictionary submatrix for active components
  auto subCols = [](const TMatrixD& W, const std::vector<int>& cols) {
    TMatrixD S(W.GetNrows(), cols.size());
    for (size_t jj = 0; jj < cols.size(); ++jj) {
      int c = cols[jj];
      for (int i = 0; i < W.GetNrows(); ++i) S(i, jj) = W(i, c);
    }
    return S;
  };

  // Iterative thresholding: remove components below threshold and re-solve
  int iter = 0;
  const int max_iter = std::min(50, K);  // Prevent excessive iterations
  int local_iterations_ran = 0;

  while (!P.empty() && iter++ < max_iter) {
    local_iterations_ran = iter;
    // Find component with minimum weight
    double minVal = std::numeric_limits<double>::infinity();
    size_t minPos = 0;
    for (size_t k = 0; k < P.size(); ++k) {
      double v = h_full(P[k]);
      if (v < minVal) {
        minVal = v;
        minPos = k;
      }
    }

    // Stop if minimum weight exceeds threshold
    if (minVal >= threshold) break;

    // Remove component with smallest weight
    h_full(P[minPos]) = 0.0;
    P.erase(P.begin() + minPos);
    if (P.empty()) {
      h_full.Zero();
      return h_full;
    }

    // Re-solve on reduced active set
    TMatrixD W_P = subCols(W_region, P);
    TVectorD h_reduced(P.size());
    h_reduced.Zero();
    h_reduced = Math::NNLS_LawsonHanson(W_P, voltVec, epsilon, 0);

    // Update full weight vector
    h_full.Zero();
    for (size_t k = 0; k < P.size(); ++k) {
      h_full(P[k]) = h_reduced(k);
    }
  }

  // Ensure numerical stability (guard against small negative values)
  for (int j = 0; j < K; ++j) {
    if (h_full(j) < 0.0) h_full(j) = 0.0;
  }

  // Calculate chi-squared goodness of fit for solution quality assessment
  TVectorD fitted = W_region * h_full;  // Fitted waveform: W * h
  double chi2_sum = 0.0;
  for (int i = 0; i < D; ++i) {
    double residual = voltVec(i) - fitted(i);  // Data - model residual
    chi2_sum += residual * residual;
  }

  // Compute degrees of freedom (data points - active parameters)
  int active_components = 0;
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) active_components++;
  }
  int dof = std::max(1, D - active_components);  // Prevent division by zero
  chi2ndf = chi2_sum / dof;
  iterations_ran = local_iterations_ran;

  return h_full;
}

std::vector<std::pair<int, int>> WaveformAnalysisRSNNLS::FindThresholdRegions(const std::vector<double>& voltWfm,
                                                                              double threshold) {
  /**
   * Find contiguous regions where waveform is below voltage threshold.
   * Returns vector of (start_sample, end_sample) pairs with some padding.
   *
   * For PMT signals (negative-going pulses), we look for regions where
   * voltage < threshold (i.e., signal magnitude exceeds threshold).
   */

  std::vector<std::pair<int, int>> regions;

  bool in_region = false;
  int region_start = -1;
  // Use fixed padding instead of upsample_factor to avoid overly large padding
  const int padding = 1;  // ~1 samples padding (conservative)

  for (size_t i = 0; i < voltWfm.size(); ++i) {
    if (voltWfm[i] < threshold) {
      // Below threshold (signal present)
      if (!in_region) {
        // Start of new region - apply padding but don't go below 0
        region_start = std::max(0, static_cast<int>(i) - padding);
        in_region = true;
      }
    } else {
      // Above threshold (no signal)
      if (in_region) {
        // End of current region - apply padding but don't exceed waveform length
        int region_end = std::min(static_cast<int>(voltWfm.size()) - 1, static_cast<int>(i) + padding - 1);
        regions.emplace_back(region_start, region_end);
        in_region = false;
      }
    }
  }

  // Handle case where region extends to end of waveform
  if (in_region) {
    regions.emplace_back(region_start, static_cast<int>(voltWfm.size()) - 1);
  }

  // Optional: merge regions that are very close together to avoid redundant processing
  if (regions.size() > 1) {
    std::vector<std::pair<int, int>> merged_regions;
    merged_regions.push_back(regions[0]);

    for (size_t i = 1; i < regions.size(); ++i) {
      // If current region starts within padding distance of previous region end, merge them
      // Use smaller merge distance to preserve timing resolution
      if (regions[i].first <= merged_regions.back().second + padding) {
        merged_regions.back().second = regions[i].second;
      } else {
        merged_regions.push_back(regions[i]);
      }
    }
    regions = merged_regions;
  }

  return regions;
}

void WaveformAnalysisRSNNLS::ProcessThresholdRegion(const std::vector<double>& voltWfm, int start_sample,
                                                    int end_sample, DS::WaveformAnalysisResult* fit_result) {
  /**
   * Process a single threshold crossing region with rsNNLS.
   *
   * Key improvements for region-based processing:
   * - Only uses dictionary templates whose peaks could contribute to this region
   * - Proper LogNormal peak positioning ensures templates align with signal
   * - Includes tail padding to capture templates with peaks slightly outside region
   * - Direct mapping from dictionary index to PE peak time
   */

  const int region_length = end_sample - start_sample + 1;

  // Extract waveform segment for this region
  std::vector<double> region_wfm(region_length);
  for (int i = 0; i < region_length; ++i) {
    region_wfm[i] = voltWfm[start_sample + i];
  }

  // Build dictionary submatrix for this region
  // We need templates whose effective peak times could contribute to this time window
  // Dictionary column j has delay = j * digitizer_period / upsample_factor
  // Effective peak time = delay + vpe_scale

  const double start_time = start_sample * digitizer_period;
  const double end_time = end_sample * digitizer_period;

  // Calculate dictionary column range - no padding to completely eliminate edge artifacts
  // Only include templates whose PEAKS are exactly within this region time window
  // No tail padding - template selection matches region bounds exactly

  double min_desired_pe_time = start_time;  // No padding
  double max_desired_pe_time = end_time;    // No padding

  // Convert PE times back to required delays
  double min_delay = min_desired_pe_time - vpe_scale;
  double max_delay = max_desired_pe_time - vpe_scale;

  // Convert delays to dictionary column indices
  const int dict_start = std::max(0, static_cast<int>(min_delay * upsample_factor / digitizer_period));
  const int dict_end = std::min(fW.GetNcols() - 1, static_cast<int>(max_delay * upsample_factor / digitizer_period));
  const int dict_cols = dict_end - dict_start + 1;

  if (dict_cols <= 0) {
    return;  // No valid dictionary columns for this region
  }

  // Extract relevant dictionary submatrix with proper bounds checking
  TMatrixD W_region(region_length, dict_cols);
  W_region.Zero();  // Initialize to zero

  for (int row = 0; row < region_length; ++row) {
    int global_row = start_sample + row;
    if (global_row >= fW.GetNrows()) continue;  // Skip if beyond waveform

    for (int col = 0; col < dict_cols; ++col) {
      int global_col = dict_start + col;
      if (global_col >= 0 && global_col < fW.GetNcols()) {
        W_region(row, col) = fW(global_row, global_col);
      }
      // If global_col is out of bounds, W_region(row, col) remains 0.0 from initialization
    }
  }

  // Convert region waveform to TVectorD
  TVectorD region_vec(region_length);
  for (int i = 0; i < region_length; ++i) {
    region_vec(i) = region_wfm[i];
  }

  // Perform reverse sparse NNLS with iterative thresholding on this region
  TVectorD region_weights = Thresholded_rsNNLS_Region(W_region, region_vec, weight_threshold);

  // Extract PEs from significant weights
  for (int i = 0; i < dict_cols; ++i) {
    if (region_weights(i) > 0.0) {
      // Convert local dictionary index to global dictionary index
      int global_dict_index = dict_start + i;

      // PE time = delay + vpe_scale (simple approach that worked before)
      double delay = global_dict_index * digitizer_period / upsample_factor;
      double pe_time = delay + vpe_scale;

      // Sanity check: PE time should be reasonably close to the region time range
      double region_start_time = start_sample * digitizer_period;
      double region_end_time = end_sample * digitizer_period;

      // Allow some tolerance but flag obviously wrong times
      if (pe_time < region_start_time - 3.0 * vpe_scale || pe_time > region_end_time + 3.0 * vpe_scale) {
        warn << "WaveformAnalysisRSNNLS: PE time " << pe_time << " ns outside expected range ["
             << (region_start_time - 3.0 * vpe_scale) << ", " << (region_end_time + 3.0 * vpe_scale) << "] for region ["
             << start_sample << ", " << end_sample << "]" << newline;
        continue;  // Skip this PE
      }

      // Convert NNLS weight to charge
      double pe_charge = region_weights(i) * vpe_scale / fTermOhms;

      // Store PE with individual weight as FOM
      fit_result->AddPE(pe_time, pe_charge,
                        {
                            {"chi2ndf", chi2ndf},
                            {"iterations_ran", iterations_ran},
                            {"weight", region_weights(i)},
                            {"index", static_cast<double>(global_dict_index)},
                            {"region_start", static_cast<double>(start_sample)},
                        });
    }
  }
}
}  // namespace RAT