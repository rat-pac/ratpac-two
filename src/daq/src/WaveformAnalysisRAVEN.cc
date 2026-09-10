#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/NPEEstimator.hh>
#include <RAT/WaveformAnalysisRAVEN.hh>
#include <algorithm>
#include <cmath>
#include <limits>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/NNLS.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

namespace {
// Dictionary column j is the template delayed by j digitizer periods over the upsampling
// factor, so only a whole-number factor puts every column on a common integer lag grid.
void ValidateUpsampleFactor(double value) {
  if (value <= 0.0 || std::abs(value - std::lround(value)) > 1e-9) {
    RAT::Log::Die("WaveformAnalysisRAVEN: Invalid upsampling factor " + std::to_string(value) +
                  ". Must be a positive whole number.");
  }
}

// PMTPULSE stores the gaussian width as a piecewise-linear PDF that the waveform generator
// samples by inverse-CDF. RAVEN fits one template per PMT, so collapse the PDF to its median by
// inverting the same trapezoidal CDF. The median is better for skewed width distributions.
double PDFMedian(const std::vector<double>& x, const std::vector<double>& prob) {
  if (x.size() != prob.size() || x.size() < 2) {
    RAT::Log::Die(
        "WaveformAnalysisRAVEN: PMTPULSE width PDF needs at least two matching width and probability points.");
  }

  std::vector<double> cumu(x.size(), 0.0);
  for (size_t i = 0; i + 1 < x.size(); ++i) {
    cumu[i + 1] = cumu[i] + (x[i + 1] - x[i]) * (prob[i] + prob[i + 1]) / 2.0;
  }

  if (cumu.back() <= 0.0) {
    RAT::Log::Die("WaveformAnalysisRAVEN: PMTPULSE width PDF has non-positive normalization.");
  }

  for (size_t i = 1; i < x.size(); ++i) {
    const double c = cumu[i] / cumu.back();
    if (0.5 <= c) {
      const double c_prev = cumu[i - 1] / cumu.back();
      return (0.5 - c_prev) * (x[i] - x[i - 1]) / (c - c_prev) + x[i - 1];
    }
  }
  return x.back();
}

// Cache key for the single template every channel shares when per-PMT shapes are off. PMT ids
// are non-negative, so this cannot collide with one.
constexpr int kSharedTemplate = -1;
}  // namespace

void WaveformAnalysisRAVEN::Configure(const std::string& config_name) {
  debug << "WaveformAnalysisRAVEN: Configure called with config_name " << config_name << newline;
  // Load analysis parameters from DIGITIZER_ANALYSIS database
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);

    // Threshold crossing region configuration
    process_threshold_crossing = (fDigit->GetI("process_threshold_crossing") != 0);  // 0=no, 1=yes
    if (process_threshold_crossing) {
      voltage_threshold = fDigit->GetD("voltage_threshold");      // Voltage threshold in mV
      threshold_region_padding = fDigit->GetI("region_padding");  // Padding samples around threshold crossing
    }

    // Template type configuration
    template_type = fDigit->GetI("raven_template_type");  // 0=lognormal, 1=gaussian

    // Single photoelectron waveform parameters
    if (template_type == 0) {                             // lognormal
      lognormal_scale = fDigit->GetD("lognormal_scale");  // LogNormal 'm' parameter
      lognormal_shape = fDigit->GetD("lognormal_shape");  // LogNormal 'sigma' parameter
    } else if (template_type == 1) {                      // gaussian
      gaussian_width = fDigit->GetD("gaussian_width");    // Gaussian 'sigma' parameter
    } else {
      RAT::Log::Die("WaveformAnalysisRAVEN: Invalid template_type " + std::to_string(template_type) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }

    vpe_charge = fDigit->GetD("vpe_charge");  // Nominal PE charge in pC

    // Algorithm configuration
    max_iterations = fDigit->GetI("max_iterations");      // Max thresholding iterations
    weight_threshold = fDigit->GetD("weight_threshold");  // Component significance threshold
    upsample_factor = fDigit->GetD("upsampling_factor");  // Dictionary upsampling factor
    epsilon = fDigit->GetD("nnls_tolerance");             // NNLS convergence tolerance

    // NPE estimation configuration
    npe_estimate = fDigit->GetZ("npe_estimate");
    npe_estimate_charge_width = fDigit->GetD("npe_estimate_charge_width");
    npe_estimate_max_pes = fDigit->GetI("npe_estimate_max_pes");

    // Weight merging configuration
    weight_merge_window = fDigit->GetD("weight_merge_window");  // Time window for merging nearby weights (ns)

    // Optional time refinement
    refine_times = false;
    try {
      refine_times = fDigit->GetZ("refine_times");
    } catch (DBWrongTypeError&) {
      RAT::Log::Die("WaveformAnalysisRAVEN: refine_times must be a boolean (true/false), not an integer.");
    } catch (DBNotFoundError&) {
    }

    // Optional per-PMT template shapes from PMTPULSE
    width_from_pmtpulse = true;
    try {
      width_from_pmtpulse = fDigit->GetZ("width_from_pmtpulse");
    } catch (DBWrongTypeError&) {
      RAT::Log::Die("WaveformAnalysisRAVEN: width_from_pmtpulse must be a boolean (true/false), not an integer.");
    } catch (DBNotFoundError&) {
    }

    // Validate critical parameters
    ValidateUpsampleFactor(upsample_factor);

    // Initialize dictionary flags
    ClearTemplateCache();
    cached_nsamples = -1;            // Invalid initial value to force dictionary build on first use
    cached_digitizer_period = -1.0;  // Invalid initial value to force dictionary build on first use

  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisRAVEN: Unable to find analysis parameters.");
  }
}

// Parameters that change the template shape or scale invalidate the cached templates;
// they are rebuilt on the next waveform.
void WaveformAnalysisRAVEN::SetD(std::string param, double value) {
  if (param == "lognormal_scale") {
    lognormal_scale = value;
    ClearTemplateCache();
  } else if (param == "lognormal_shape") {
    lognormal_shape = value;
    ClearTemplateCache();
  } else if (param == "gaussian_width") {
    gaussian_width = value;
    ClearTemplateCache();
  } else if (param == "vpe_charge") {
    vpe_charge = value;
    ClearTemplateCache();
  } else if (param == "upsampling_factor") {
    ValidateUpsampleFactor(value);
    upsample_factor = value;
    ClearTemplateCache();
  } else if (param == "weight_threshold") {
    weight_threshold = value;
  } else if (param == "voltage_threshold") {
    voltage_threshold = value;
  } else if (param == "nnls_tolerance") {
    epsilon = value;
  } else if (param == "npe_estimate_charge_width") {
    npe_estimate_charge_width = value;
  } else if (param == "weight_merge_window") {
    weight_merge_window = value;
  } else {
    WaveformAnalyzerBase::SetD(param, value);
  }
}

void WaveformAnalysisRAVEN::SetI(std::string param, int value) {
  if (param == "process_threshold_crossing") {
    process_threshold_crossing = (value != 0);
  } else if (param == "max_iterations") {
    max_iterations = value;
  } else if (param == "raven_template_type") {
    template_type = value;
    if (template_type != 0 && template_type != 1) {
      RAT::Log::Die("WaveformAnalysisRAVEN: Invalid raven_template_type " + std::to_string(value) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }
    ClearTemplateCache();
  } else if (param == "npe_estimate") {
    npe_estimate = (value != 0);
  } else if (param == "npe_estimate_max_pes") {
    npe_estimate_max_pes = static_cast<size_t>(value);
  } else if (param == "refine_times") {
    refine_times = (value != 0);
  } else if (param == "width_from_pmtpulse") {
    width_from_pmtpulse = (value != 0);
    ClearTemplateCache();
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisRAVEN::ClearTemplateCache() {
  fTemplateCache.clear();
  fModelShapeCache.clear();
  dictionary_built = false;
}

WaveformAnalysisRAVEN::TemplateShape WaveformAnalysisRAVEN::ConfiguredShape() const {
  // The gaussian template has no shape parameter, and Configure leaves lognormal_shape unset
  // whenever that template is selected, so the shape stays zero here: reading it would be an
  // uninitialized read.
  return (template_type == 0) ? TemplateShape(lognormal_shape, lognormal_scale) : TemplateShape(0.0, gaussian_width);
}

// RAVEN's template is the waveform generator's pulse shape, so its per-model parameters come
// from the generator's own PMTPULSE table. There 'lognormal_width' and 'lognormal_mean' are
// RAVEN's shape and scale, and the gaussian width is a PDF rather than a single value.
WaveformAnalysisRAVEN::TemplateShape WaveformAnalysisRAVEN::ShapeForModel(const std::string& model_name) {
  std::map<std::string, TemplateShape>::const_iterator cached = fModelShapeCache.find(model_name);
  if (cached != fModelShapeCache.end()) {
    return cached->second;
  }

  DBLinkPtr lpulse;
  try {
    lpulse = DB::Get()->GetLink("PMTPULSE", model_name);
    lpulse->GetS("index");
  } catch (DBNotFoundError&) {
    // The generator falls back to the default entry for an unlisted model; match it.
    try {
      lpulse = DB::Get()->GetLink("PMTPULSE", "");
      lpulse->GetS("index");
    } catch (DBNotFoundError&) {
      lpulse = nullptr;
    }
  }

  // Fall back to the configured parameters whenever PMTPULSE cannot describe this model with
  // the template RAVEN is set up to fit.
  TemplateShape shape = ConfiguredShape();
  std::string reason;

  if (!lpulse) {
    reason = "no PMTPULSE entry";
  } else {
    try {
      const std::string pulse_type = lpulse->GetS("pulse_type");
      const std::string pulse_shape = lpulse->GetS("pulse_shape");
      const std::string wanted = (template_type == 0) ? "lognormal" : "gaussian";

      if (pulse_type != "analytic") {
        reason = "PMTPULSE pulse_type is " + pulse_type + ", not analytic";
      } else if (pulse_shape != wanted) {
        reason = "PMTPULSE pulse_shape is " + pulse_shape + ", but RAVEN fits " + wanted;
      } else if (template_type == 0) {
        shape = TemplateShape(lpulse->GetD("lognormal_width"), lpulse->GetD("lognormal_mean"));
      } else {
        shape.second = PDFMedian(lpulse->GetDArray("gaussian_width"), lpulse->GetDArray("gaussian_width_prob"));
      }
    } catch (DBNotFoundError&) {
      reason = "PMTPULSE entry is missing template parameters";
    }
  }

  if (reason.empty()) {
    info << "WaveformAnalysisRAVEN: Template for " << model_name
         << " from PMTPULSE: " << (template_type == 0 ? "lognormal m " : "gaussian sigma ") << shape.second << " ns"
         << newline;
  } else {
    warn << "WaveformAnalysisRAVEN: " << reason << " for " << model_name
         << ". Using the configured template parameters." << newline;
  }

  fModelShapeCache[model_name] = shape;
  return shape;
}

WaveformAnalysisRAVEN::TemplateShape WaveformAnalysisRAVEN::ShapeForPMT(int pmtid) {
  DS::Run* run = DS::RunStore::GetCurrentRun();
  TemplateShape shape = ShapeForModel(run->GetPMTInfo()->GetModelNameByID(pmtid));
  // The generator scales the lognormal 'm' and the gaussian width per channel, leaving the
  // lognormal 'sigma' alone.
  shape.second *= run->GetChannelStatus()->GetPulseWidthScaleByPMTID(pmtid);
  return shape;
}

const std::vector<double>& WaveformAnalysisRAVEN::GetTemplateProfile(int key, const TemplateShape& shape) {
  std::map<int, std::vector<double>>::iterator cached = fTemplateCache.find(key);
  if (cached != fTemplateCache.end()) {
    return cached->second;
  }

  std::vector<double>& profile = fTemplateCache[key];
  BuildTemplateProfile(shape, profile);

  debug << "WaveformAnalysisRAVEN: Built a " << profile.size() << " point profile for scale " << shape.second << " ("
        << fTemplateCache.size() << " cached)" << newline;

  return profile;
}

// The dictionary is shift invariant: an entry depends on its row and column only through
// the lag between the sample and the template start. One template sampled on that lag grid
// therefore generates every column, at 2 * upsample_factor entries per sample rather than
// the nsamples * upsample_factor of the full matrix.
void WaveformAnalysisRAVEN::BuildTemplateProfile(const TemplateShape& shape, std::vector<double>& profile) const {
  const double template_shape = shape.first;
  const double template_scale = shape.second;

  profile.assign(profile_offset + (cached_nsamples - 1) * cached_upsample + 1, 0.0);

  const double mag_factor = vpe_charge * fTermOhms;

  for (size_t i = 0; i < profile.size(); ++i) {
    const double lag = (static_cast<int>(i) - profile_offset) * cached_digitizer_period / upsample_factor;
    double template_val = 0.0;

    if (template_type == 0) {  // lognormal
      if (lag > -template_scale) {
        template_val = mag_factor * TMath::LogNormal(lag, template_shape, -template_scale, template_scale);
      }
    } else if (template_type == 1) {  // gaussian
      template_val = mag_factor * TMath::Gaus(lag, 0.0, template_scale, kTRUE);
    }

    profile[i] = -template_val;
  }
}

void WaveformAnalysisRAVEN::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  // Lay out the lag grid on first call or when digitizer parameters change. The cached
  // templates are sampled on that grid, so they are dropped with it.
  const double period_tolerance = 1e-9;  // 1 ps tolerance for digitizer period comparison
  if (!dictionary_built || cached_nsamples != static_cast<int>(digitWfm.size()) ||
      std::abs(cached_digitizer_period - fTimeStep) > period_tolerance) {
    ClearTemplateCache();

    // Use current digitizer information from the waveform and base class
    cached_nsamples = static_cast<int>(digitWfm.size());
    cached_digitizer_period = fTimeStep;
    cached_upsample = static_cast<int>(std::lround(upsample_factor));
    cached_dict_size = cached_nsamples * cached_upsample;

    // The lag index row * upsample - col runs from -(dict_size - 1) to (nsamples - 1) * upsample;
    // profile_offset shifts it onto a non-negative array index.
    profile_offset = cached_dict_size - 1;
    dictionary_built = true;

    debug << "WaveformAnalysisRAVEN: Lag grid for a " << cached_nsamples << " x " << cached_dict_size
          << " dictionary, raven_template_type " << template_type << " ("
          << (template_type == 0 ? "lognormal" : "gaussian") << ")" << newline;
  }

  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisRAVEN: Pedestal is invalid! Did you run WaveformPrep first?");
  }

  // Get per-PMT gain calibration for consistent charge calculation (same as LucyDDM)
  double gain_calibration = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetChargeScaleByPMTID(digitpmt->GetID());

  // Pick this PMT's template, cached per channel. With per-PMT shapes off every channel fits
  // the same configured template, so they all share one entry.
  const int pmtid = digitpmt->GetID();
  const TemplateShape shape = width_from_pmtpulse ? ShapeForPMT(pmtid) : ConfiguredShape();
  const std::vector<double>& profile = GetTemplateProfile(width_from_pmtpulse ? pmtid : kSharedTemplate, shape);

  // Verify waveform size matches the lag grid the templates were sampled on
  if (static_cast<int>(digitWfm.size()) != cached_nsamples) {
    RAT::Log::Die("WaveformAnalysisRAVEN: Waveform size mismatch with dictionary.");
  }

  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult(GetAnalyzerName());

  if (process_threshold_crossing) {
    // Find threshold crossing regions
    std::vector<std::pair<int, int>> crossing_regions =
        FindThresholdRegions(voltWfm, voltage_threshold, threshold_region_padding);

    if (crossing_regions.empty()) {
      // No signal above threshold - return empty result
      return;
    }

    // Process each threshold crossing region independently

    for (const auto& region : crossing_regions) {
      int start_sample = region.first;
      int end_sample = region.second;

      // Perform rsNNLS on this region
      ProcessThresholdRegion(profile, voltWfm, start_sample, end_sample, fit_result, gain_calibration);
    }
  } else {
    int start_sample = 0;
    int end_sample = static_cast<int>(voltWfm.size()) - 1;

    // Perform rsNNLS on the entire waveform
    ProcessThresholdRegion(profile, voltWfm, start_sample, end_sample, fit_result, gain_calibration);
  }
}

TVectorD WaveformAnalysisRAVEN::Thresholded_rsNNLS(const TMatrixD& W_region, const TVectorD& voltVec,
                                                   const double threshold, double& chi2ndf_out, int& iterations_out) {
  const int D = voltVec.GetNrows();
  const int K = W_region.GetNcols();

  if (W_region.GetNrows() != D) {
    RAT::Log::Die("WaveformAnalysisRAVEN: Dictionary region row dimension mismatch.");
  }

  // Initial NNLS solve
  TVectorD h_full(K);
  h_full.Zero();
  h_full = Math::NNLS_LawsonHanson(W_region, voltVec, epsilon, 0, 0);

  // Build initial active set
  std::vector<int> P;
  P.reserve(K);
  for (int j = 0; j < K; ++j) {
    if (h_full(j) > 0.0) P.push_back(j);
  }

  // A threshold-crossing region exists because the waveform crossed threshold, so
  // it must yield a PE. The solver can still return nothing when nnls_tolerance is
  // set above the pulse's own gradient, so seed the best-correlated column with the
  // weight NNLS would give it alone: max(0, A_j.v / ||A_j||^2).
  if (process_threshold_crossing && P.empty()) {
    int seed_col = -1;
    double seed_dot = 0.0;
    for (int j = 0; j < K; ++j) {
      double dot = 0.0;
      for (int i = 0; i < D; ++i) dot += W_region(i, j) * voltVec(i);
      if (dot > seed_dot) {
        seed_dot = dot;
        seed_col = j;
      }
    }
    if (seed_col >= 0) {
      double norm2 = 0.0;
      for (int i = 0; i < D; ++i) norm2 += W_region(i, seed_col) * W_region(i, seed_col);
      if (norm2 > 0.0) {
        h_full(seed_col) = seed_dot / norm2;
        P.push_back(seed_col);
      }
    }
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

  int local_iterations_ran = 0;

  // Iterative thresholding. Time refinement runs a second pass over this, and both passes
  // draw on one iteration budget, which local_iterations_ran carries between them.
  auto pruneBelowThreshold = [&]() {
    while (local_iterations_ran < static_cast<int>(max_iterations) && !P.empty()) {
      local_iterations_ran++;

      // Find component with minimum weight
      std::vector<int>::iterator minIt =
          std::min_element(P.begin(), P.end(), [&h_full](int a, int b) { return h_full(a) < h_full(b); });
      size_t minPos = std::distance(P.begin(), minIt);
      double minVal = h_full(*minIt);

      if (minVal >= threshold) break;

      // Never prune the last remaining component: always fit at least one PE per threshold crossing
      if (P.size() == 1) break;

      // Remove component with smallest weight
      h_full(P[minPos]) = 0.0;
      P.erase(P.begin() + minPos);

      // Re-solve on reduced active set
      TMatrixD W_P = subCols(W_region, P);
      TVectorD h_reduced(P.size());
      h_reduced.Zero();
      h_reduced = Math::NNLS_LawsonHanson(W_P, voltVec, epsilon, 0, 0);

      // Update full weight vector
      h_full.Zero();
      for (size_t k = 0; k < P.size(); ++k) {
        h_full(P[k]) = h_reduced(k);
      }
    }
  };

  pruneBelowThreshold();

  // Time refinement. Reverse pursuit only removes components, so it cannot correct one the
  // initial solve misplaced, typically by ~1 sample on a steep leading edge. Refinement moves
  // such a component onto the column that fits it, clearing the early ghost PE it would emit.
  if (refine_times && !P.empty()) {
    const int max_shift = std::max(1, static_cast<int>(std::lround(upsample_factor)));  // +- 1 sample
    auto residualOf = [&](const TVectorD& h) {
      TVectorD r = voltVec;
      for (int j = 0; j < K; ++j) {
        if (h(j) == 0.0) continue;
        for (int i = 0; i < D; ++i) r(i) -= W_region(i, j) * h(j);
      }
      return r;
    };
    TVectorD r_full = residualOf(h_full);
    double cur_rss = r_full * r_full;
    std::vector<char> occupied(K, 0);
    for (int c : P) occupied[c] = 1;

    // Column norms are fixed across candidates and sweeps, so compute each once.
    std::vector<double> col_norm2(K, -1.0);
    auto colNorm2 = [&](int col) {
      if (col_norm2[col] < 0.0) {
        double n2 = 0.0;
        for (int i = 0; i < D; ++i) n2 += W_region(i, col) * W_region(i, col);
        col_norm2[col] = n2;
      }
      return col_norm2[col];
    };

    // Two sweeps at most: a move can free a column its neighbour then wants, but in
    // practice nothing moves after the second pass and each sweep costs a full re-solve.
    for (int sweep = 0; sweep < 2; ++sweep) {
      bool improved = false;
      for (size_t idx = 0; idx < P.size(); ++idx) {
        const int c = P[idx];
        // Residual with this component removed, other weights fixed.
        TVectorD r_wo = r_full;
        for (int i = 0; i < D; ++i) r_wo(i) += W_region(i, c) * h_full(c);
        const double rss_wo = r_wo * r_wo;
        // Score each nearby free column by the rss left after refitting one weight on it.
        int best_col = c;
        double best_1d = std::numeric_limits<double>::max();
        for (int dc = -max_shift; dc <= max_shift; ++dc) {
          const int cp = c + dc;
          if (cp < 0 || cp >= K) continue;
          if (cp != c && occupied[cp]) continue;
          double dot = 0.0;
          for (int i = 0; i < D; ++i) dot += W_region(i, cp) * r_wo(i);
          const double norm2 = colNorm2(cp);
          const double gain = (dot > 0.0 && norm2 > 0.0) ? dot * dot / norm2 : 0.0;
          const double rss_1d = rss_wo - gain;
          if (rss_1d < best_1d - 1e-12) {
            best_1d = rss_1d;
            best_col = cp;
          }
        }
        if (best_col == c) continue;

        // Full re-solve with the component moved; accept only on improvement.
        std::vector<int> P_trial = P;
        P_trial[idx] = best_col;
        TMatrixD W_P = subCols(W_region, P_trial);
        TVectorD h_trial = Math::NNLS_LawsonHanson(W_P, voltVec, epsilon, 0, 0);
        TVectorD h_new(K);
        h_new.Zero();
        for (size_t k = 0; k < P_trial.size(); ++k) h_new(P_trial[k]) = h_trial(static_cast<int>(k));
        TVectorD r_new = residualOf(h_new);
        const double new_rss = r_new * r_new;
        // Relative test: an absolute one would mean different things per region size.
        if (new_rss < cur_rss * (1.0 - 1e-9)) {
          occupied[c] = 0;
          occupied[best_col] = 1;
          P[idx] = best_col;
          h_full = h_new;
          r_full = r_new;
          cur_rss = new_rss;
          improved = true;
        }
      }
      if (!improved) break;
    }

    // Re-solving on a moved support can drop a survivor back below `threshold`, so re-apply
    // the cut.
    pruneBelowThreshold();
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
  chi2ndf_out = chi2_sum / dof;
  iterations_out = local_iterations_ran;

  return h_full;
}

std::vector<std::pair<int, int>> WaveformAnalysisRAVEN::FindThresholdRegions(const std::vector<double>& voltWfm,
                                                                             double threshold, int region_padding) {
  std::vector<std::pair<int, int>> regions;
  bool in_region = false;
  int region_start = -1;

  for (size_t i = 0; i < voltWfm.size(); ++i) {
    // Falling edge detection - signal goes below threshold
    if (voltWfm[i] < threshold && !in_region) {
      region_start = std::max(0, static_cast<int>(i) - region_padding);
      in_region = true;
    }
    // Rising edge detection - signal goes above threshold
    else if (voltWfm[i] >= threshold && in_region) {
      int region_end = std::min(static_cast<int>(voltWfm.size()) - 1, static_cast<int>(i) + region_padding - 1);
      regions.emplace_back(region_start, region_end);
      in_region = false;
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
      if (regions[i].first <= merged_regions.back().second + region_padding) {
        merged_regions.back().second = regions[i].second;
      } else {
        merged_regions.push_back(regions[i]);
      }
    }
    std::swap(regions, merged_regions);
  }

  return regions;
}

void WaveformAnalysisRAVEN::ProcessThresholdRegion(const std::vector<double>& profile,
                                                   const std::vector<double>& voltWfm, int start_sample, int end_sample,
                                                   DS::WaveformAnalysisResult* fit_result, double gain_calibration) {
  const int region_length = end_sample - start_sample + 1;

  // Use iterators to avoid copying waveform segment
  std::vector<double>::const_iterator region_begin = voltWfm.begin() + start_sample;

  // Calculate dictionary column range directly from sample indices
  // Dictionary column j corresponds to sample time j/upsample_factor
  // We want columns that correspond to this region's sample range

  const int dict_start = std::max(0, static_cast<int>(start_sample * upsample_factor));
  const int dict_end = std::min(cached_dict_size - 1, static_cast<int>(end_sample * upsample_factor));
  const int dict_cols = dict_end - dict_start + 1;

  if (dict_cols <= 0) {
    return;
  }

  // Build this region's dictionary submatrix from this PMT's template profile
  TMatrixD W_region(region_length, dict_cols);
  W_region.Zero();

  for (int row = 0; row < region_length; ++row) {
    int global_row = start_sample + row;
    if (global_row >= cached_nsamples) continue;

    // Entry (row, col) is the profile at lag global_row * upsample - (dict_start + col)
    const int lag_index = global_row * cached_upsample + profile_offset - dict_start;
    for (int col = 0; col < dict_cols; ++col) {
      W_region(row, col) = profile[lag_index - col];
    }
  }

  // Convert region waveform to TVectorD using iterators
  TVectorD region_vec(region_length);
  std::vector<double>::const_iterator it = region_begin;
  for (int i = 0; i < region_length; ++i, ++it) {
    region_vec(i) = *it;
  }

  // Perform rsNNLS on this region
  double chi2ndf;
  int iterations_ran;
  TVectorD region_weights = Thresholded_rsNNLS(W_region, region_vec, weight_threshold, chi2ndf, iterations_ran);

  // Extract PEs from significant weights
  ExtractPhotoelectrons(region_weights, dict_start, dict_cols, start_sample, end_sample, chi2ndf, iterations_ran,
                        fit_result, gain_calibration);
}

std::vector<std::pair<double, double>> WaveformAnalysisRAVEN::MergeNearbyWeights(const TVectorD& region_weights,
                                                                                 int dict_start, int dict_cols,
                                                                                 double merge_window) {
  // Collect non-zero weights with their times
  std::vector<std::pair<double, double>> time_weight_pairs;  // (time, weight)
  for (int i = 0; i < dict_cols; ++i) {
    if (region_weights(i) > 0.0) {
      int global_dict_index = dict_start + i;
      double time = global_dict_index * fTimeStep / upsample_factor;
      time_weight_pairs.emplace_back(time, region_weights(i));
    }
  }

  if (time_weight_pairs.empty() || merge_window <= 0.0) {
    return time_weight_pairs;
  }

  // Merge weights within the time window
  std::vector<std::pair<double, double>> merged_weights;
  merged_weights.reserve(time_weight_pairs.size());

  size_t i = 0;
  while (i < time_weight_pairs.size()) {
    // Start a new cluster
    double cluster_weight_sum = time_weight_pairs[i].second;
    double cluster_time_weighted_sum = time_weight_pairs[i].first * time_weight_pairs[i].second;
    size_t cluster_end = i + 1;

    // Extend cluster to include all weights within merge_window of any cluster member
    while (cluster_end < time_weight_pairs.size()) {
      // Check if next weight is within merge_window of the cluster's weighted mean time
      double cluster_mean_time = cluster_time_weighted_sum / cluster_weight_sum;
      if (time_weight_pairs[cluster_end].first - cluster_mean_time <= merge_window) {
        // Add to cluster
        cluster_weight_sum += time_weight_pairs[cluster_end].second;
        cluster_time_weighted_sum += time_weight_pairs[cluster_end].first * time_weight_pairs[cluster_end].second;
        cluster_end++;
      } else {
        break;
      }
    }

    // Store merged weight with charge-weighted mean time
    double merged_time = cluster_time_weighted_sum / cluster_weight_sum;
    merged_weights.emplace_back(merged_time, cluster_weight_sum);

    i = cluster_end;
  }

  return merged_weights;
}

void WaveformAnalysisRAVEN::ExtractPhotoelectrons(const TVectorD& region_weights, int dict_start, int dict_cols,
                                                  int start_sample, int end_sample, double chi2ndf, int iterations_ran,
                                                  DS::WaveformAnalysisResult* fit_result, double gain_calibration) {
  // Merge nearby weights to prevent PE overcounting from weight splitting
  std::vector<std::pair<double, double>> merged_weights =
      MergeNearbyWeights(region_weights, dict_start, dict_cols, weight_merge_window);

  // A merged time is a weight-weighted mean of this region's own dictionary column times, and
  // refinement only moves components between those columns, so a time outside the column span
  // is an indexing error. One column of slack absorbs rounding in the mean.
  const double column_step = fTimeStep / upsample_factor;
  const double earliest_time = (dict_start - 1) * column_step;
  const double latest_time = (dict_start + dict_cols) * column_step;

  // Extract PEs from merged weights
  for (const auto& [delay, weight] : merged_weights) {
    if (delay < earliest_time || delay > latest_time) {
      warn << "WaveformAnalysisRAVEN: PE time " << delay << " ns outside the dictionary columns [" << earliest_time
           << ", " << latest_time << "] of region [" << start_sample << ", " << end_sample << "]" << newline;
      continue;
    }

    // Charge calculation: apply gain calibration
    double pe_charge = weight * vpe_charge * gain_calibration;  // Charge in pC

    // Estimate number of PEs using likelihood method
    // Use calibrated vpe_charge so expected charge per PE matches the scale of pe_charge
    double calibrated_vpe_charge = vpe_charge * gain_calibration;
    size_t npe = npe_estimate
                     ? EstimateNPE(pe_charge, calibrated_vpe_charge, npe_estimate_charge_width, npe_estimate_max_pes)
                     : 1;

    // Add each estimated PE with divided charge
    for (size_t ipe = 0; ipe < npe; ++ipe) {
      fit_result->AddPE(delay, pe_charge / npe,
                        {
                            {"chi2ndf", chi2ndf},
                            {"iterations_ran", static_cast<double>(iterations_ran)},
                            {"weight", weight / npe},
                            {"estimated_npe", static_cast<double>(npe)},
                        });
    }
  }
}
}  // namespace RAT