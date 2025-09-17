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

    // Template type configuration
    template_type = fDigit->GetI("rsnnls_template_type");  // 0=lognormal, 1=gaussian

    // Single photoelectron waveform parameters
    if (template_type == 0) {                             // lognormal
      lognormal_scale = fDigit->GetD("lognormal_scale");  // LogNormal 'm' parameter
      lognormal_shape = fDigit->GetD("lognormal_shape");  // LogNormal 'sigma' parameter
    } else if (template_type == 1) {                      // gaussian
      gaussian_width = fDigit->GetD("gaussian_width");    // Gaussian 'sigma' parameter
    } else {
      RAT::Log::Die("WaveformAnalysisRSNNLS: Invalid template_type " + std::to_string(template_type) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }

    vpe_charge = fDigit->GetD("vpe_charge");  // Nominal PE charge in pC

    // Algorithm configuration
    max_iterations = fDigit->GetI("max_iterations");        // Max thresholding iterations
    weight_threshold = fDigit->GetD("weight_threshold");    // Component significance threshold
    upsample_factor = fDigit->GetD("upsampling_factor");    // Dictionary upsampling factor
    epsilon = fDigit->GetD("nnls_tolerance");               // NNLS convergence tolerance
    voltage_threshold = fDigit->GetD("voltage_threshold");  // Voltage threshold for region detection

    // Validate critical parameters
    if (upsample_factor <= 0) {
      RAT::Log::Die("WaveformAnalysisRSNNLS: Invalid upsampling factor.");
    }

    // Initialize dictionary flags
    dictionary_built = false;

  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisRSNNLS::SetD(std::string param, double value) {
  if (param == "lognormal_scale") {
    lognormal_scale = value;
  } else if (param == "lognormal_shape") {
    lognormal_shape = value;
  } else if (param == "gaussian_width") {
    gaussian_width = value;
  } else if (param == "vpe_charge") {
    vpe_charge = value;
  } else if (param == "upsampling_factor") {
    upsample_factor = value;
  } else if (param == "weight_threshold") {
    weight_threshold = value;
  } else if (param == "voltage_threshold") {
    voltage_threshold = value;
  } else if (param == "nnls_tolerance") {
    epsilon = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisRSNNLS::SetI(std::string param, int value) {
  if (param == "max_iterations") {
    max_iterations = value;
  } else if (param == "rsnnls_template_type") {
    template_type = value;
    if (template_type != 0 && template_type != 1) {
      RAT::Log::Die("WaveformAnalysisRSNNLS: Invalid rsnnls_template_type " + std::to_string(value) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisRSNNLS::BuildDictionaryMatrix(int nsamples, double digitizer_period) {
  info << "WaveformAnalysisRSNNLS: Building dictionary matrix" << newline;
  info << "WaveformAnalysisRSNNLS: Using rsnnls_template_type: " << template_type << " ("
       << (template_type == 0 ? "lognormal" : "gaussian") << ")" << newline;
  info << "WaveformAnalysisRSNNLS: Digitizer period: " << digitizer_period << " ns" << newline;
  info << "WaveformAnalysisRSNNLS: Dictionary size: " << nsamples << " x "
       << static_cast<int>(nsamples * upsample_factor) << newline;

  const int dict_size = static_cast<int>(nsamples * upsample_factor);
  fW.ResizeTo(nsamples, dict_size);
  fW.Zero();

  const double mag_factor = vpe_charge * fTermOhms;

  // Generate dictionary with time-shifted templates
  for (int col = 0; col < dict_size; ++col) {
    double delay = col * digitizer_period / upsample_factor;

    for (int row = 0; row < nsamples; ++row) {
      double sample_time = row * digitizer_period;
      double template_val = 0.0;

      if (template_type == 0) {  // lognormal
        double lognormal_shift = delay - lognormal_scale;
        if (sample_time > lognormal_shift) {
          template_val = mag_factor * TMath::LogNormal(sample_time, lognormal_shape, lognormal_shift, lognormal_scale);
        }
      } else if (template_type == 1) {  // gaussian
        template_val = mag_factor * TMath::Gaus(sample_time, delay, gaussian_width, kTRUE);
      }

      fW(row, col) = -template_val;
    }
  }
}

void WaveformAnalysisRSNNLS::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  // Build dictionary on first call or when digitizer parameters change
  if (!dictionary_built || cached_nsamples != static_cast<int>(digitWfm.size()) ||
      cached_digitizer_period != fTimeStep) {
    // Use current digitizer information from the waveform and base class
    int nsamples = static_cast<int>(digitWfm.size());
    double digitizer_period = fTimeStep;

    BuildDictionaryMatrix(nsamples, digitizer_period);

    cached_nsamples = nsamples;
    cached_digitizer_period = digitizer_period;
    dictionary_built = true;
  }

  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Pedestal is invalid! Did you run WaveformPrep first?");
  }

  // Verify waveform size matches dictionary matrix
  if (static_cast<int>(digitWfm.size()) != fW.GetNrows()) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Waveform size mismatch with dictionary matrix.");
  }

  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  // Find threshold crossing regions
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

    // Perform rsNNLS on this region
    ProcessThresholdRegion(voltWfm, start_sample, end_sample, fit_result);
  }
}

TVectorD WaveformAnalysisRSNNLS::Thresholded_rsNNLS(const TMatrixD& W_region, const TVectorD& voltVec,
                                                    const double threshold) {
  const int D = voltVec.GetNrows();
  const int K = W_region.GetNcols();

  if (W_region.GetNrows() != D) {
    RAT::Log::Die("WaveformAnalysisRSNNLS: Dictionary region row dimension mismatch.");
  }

  // Initial NNLS solve
  TVectorD h_full(K);
  h_full.Zero();
  h_full = Math::NNLS_LawsonHanson(W_region, voltVec, epsilon, 0);

  // Build initial active set
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

  // Iterative thresholding
  int iter = 0;
  const int max_iter = std::min(50, K);
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

  // Ensure numerical stability
  for (int j = 0; j < K; ++j) {
    if (h_full(j) < 0.0) h_full(j) = 0.0;
  }

  // Calculate chi-squared goodness of fit
  TVectorD fitted = W_region * h_full;
  double chi2_sum = 0.0;
  for (int i = 0; i < D; ++i) {
    double residual = voltVec(i) - fitted(i);
    chi2_sum += residual * residual;
  }

  int active_components = 0;
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) active_components++;
  }
  int dof = std::max(1, D - active_components);
  chi2ndf = chi2_sum / dof;
  iterations_ran = local_iterations_ran;

  return h_full;
}

std::vector<std::pair<int, int>> WaveformAnalysisRSNNLS::FindThresholdRegions(const std::vector<double>& voltWfm,
                                                                              double threshold) {
  std::vector<std::pair<int, int>> regions;
  bool in_region = false;
  int region_start = -1;
  const int padding = 1;

  for (size_t i = 0; i < voltWfm.size(); ++i) {
    if (voltWfm[i] < threshold) {
      if (!in_region) {
        region_start = std::max(0, static_cast<int>(i) - padding);
        in_region = true;
      }
    } else {
      if (in_region) {
        int region_end = std::min(static_cast<int>(voltWfm.size()) - 1, static_cast<int>(i) + padding - 1);
        regions.emplace_back(region_start, region_end);
        in_region = false;
      }
    }
  }

  if (in_region) {
    regions.emplace_back(region_start, static_cast<int>(voltWfm.size()) - 1);
  }

  // Merge close regions
  if (regions.size() > 1) {
    std::vector<std::pair<int, int>> merged_regions;
    merged_regions.push_back(regions[0]);

    for (size_t i = 1; i < regions.size(); ++i) {
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
  const int region_length = end_sample - start_sample + 1;

  // Extract waveform segment for this region
  std::vector<double> region_wfm(region_length);
  for (int i = 0; i < region_length; ++i) {
    region_wfm[i] = voltWfm[start_sample + i];
  }

  // Calculate dictionary column range directly from sample indices
  // Dictionary column j corresponds to sample time j/upsample_factor
  // We want columns that correspond to this region's sample range

  const int dict_start = std::max(0, static_cast<int>(start_sample * upsample_factor));
  const int dict_end = std::min(fW.GetNcols() - 1, static_cast<int>(end_sample * upsample_factor));
  const int dict_cols = dict_end - dict_start + 1;

  if (dict_cols <= 0) {
    return;
  }

  // Extract relevant dictionary submatrix
  TMatrixD W_region(region_length, dict_cols);
  W_region.Zero();

  for (int row = 0; row < region_length; ++row) {
    int global_row = start_sample + row;
    if (global_row >= fW.GetNrows()) continue;

    for (int col = 0; col < dict_cols; ++col) {
      int global_col = dict_start + col;
      if (global_col >= 0 && global_col < fW.GetNcols()) {
        W_region(row, col) = fW(global_row, global_col);
      }
    }
  }

  // Convert region waveform to TVectorD
  TVectorD region_vec(region_length);
  for (int i = 0; i < region_length; ++i) {
    region_vec(i) = region_wfm[i];
  }

  // Perform rsNNLS on this region
  TVectorD region_weights = Thresholded_rsNNLS(W_region, region_vec, weight_threshold);

  // Extract PEs from significant weights
  for (int i = 0; i < dict_cols; ++i) {
    if (region_weights(i) > 0.0) {
      int global_dict_index = dict_start + i;
      double delay = global_dict_index * fTimeStep / upsample_factor;

      // Calculate PE time based on template type
      double pe_time;
      if (template_type == 0) {         // lognormal
        pe_time = delay;                // For LogNormal, delay is already the proper time
      } else if (template_type == 1) {  // gaussian
        pe_time = delay;                // For Gaussian, delay is also the peak time
      } else {
        pe_time = delay;  // Default fallback
      }

      // Sanity check - use appropriate template scale for range checking
      double template_scale = (template_type == 0) ? lognormal_scale : gaussian_width;
      double region_start_time = start_sample * fTimeStep;
      double region_end_time = end_sample * fTimeStep;

      if (pe_time < region_start_time - 3.0 * template_scale || pe_time > region_end_time + 3.0 * template_scale) {
        warn << "WaveformAnalysisRSNNLS: PE time " << pe_time << " ns outside expected range ["
             << (region_start_time - 3.0 * template_scale) << ", " << (region_end_time + 3.0 * template_scale)
             << "] for region [" << start_sample << ", " << end_sample << "]" << newline;
        continue;
      }

      double pe_charge = region_weights(i) * vpe_charge;  // Charge in pC

      fit_result->AddPE(pe_time, pe_charge,
                        {
                            {"chi2ndf", chi2ndf},
                            {"iterations_ran", iterations_ran},
                            {"weight", region_weights(i)},
                        });
    }
  }
}
}  // namespace RAT