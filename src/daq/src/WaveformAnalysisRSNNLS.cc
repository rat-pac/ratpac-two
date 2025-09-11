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
  const int dict_size = static_cast<int>(nsamples * upsample_factor);
  fW.ResizeTo(nsamples, dict_size);
  fW.Zero();

  const double mag_factor = vpe_charge * fTermOhms;

  // Generate dictionary with time-shifted LogNormal templates
  for (int col = 0; col < dict_size; ++col) {
    double delay = col * digitizer_period / upsample_factor;
    double lognormal_shift = delay - vpe_scale;

    for (int row = 0; row < nsamples; ++row) {
      double sample_time = row * digitizer_period;
      double template_val = 0.0;

      if (sample_time > lognormal_shift) {
        template_val = mag_factor * TMath::LogNormal(sample_time, vpe_shape, lognormal_shift, vpe_scale);
      }

      fW(row, col) = -template_val;  // Negative for deconvolution
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

TVectorD WaveformAnalysisRSNNLS::Thresholded_rsNNLS_Region(const TMatrixD& W_region, const TVectorD& voltVec,
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

  // Calculate dictionary column range
  const double start_time = start_sample * digitizer_period;
  const double end_time = end_sample * digitizer_period;

  double min_delay = start_time;
  double max_delay = end_time;

  const int dict_start = std::max(0, static_cast<int>(min_delay * upsample_factor / digitizer_period));
  const int dict_end = std::min(fW.GetNcols() - 1, static_cast<int>(max_delay * upsample_factor / digitizer_period));
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
  TVectorD region_weights = Thresholded_rsNNLS_Region(W_region, region_vec, weight_threshold);

  // Extract PEs from significant weights
  for (int i = 0; i < dict_cols; ++i) {
    if (region_weights(i) > 0.0) {
      int global_dict_index = dict_start + i;
      double delay = global_dict_index * digitizer_period / upsample_factor;
      double pe_time = delay;

      // Sanity check
      double region_start_time = start_sample * digitizer_period;
      double region_end_time = end_sample * digitizer_period;

      if (pe_time < region_start_time - 3.0 * vpe_scale || pe_time > region_end_time + 3.0 * vpe_scale) {
        warn << "WaveformAnalysisRSNNLS: PE time " << pe_time << " ns outside expected range ["
             << (region_start_time - 3.0 * vpe_scale) << ", " << (region_end_time + 3.0 * vpe_scale) << "] for region ["
             << start_sample << ", " << end_sample << "]" << newline;
        continue;
      }

      double pe_charge = region_weights(i) * vpe_scale / fTermOhms;

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