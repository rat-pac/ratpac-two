#include <TDecompChol.h>
#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/NPEEstimator.hh>
#include <RAT/WaveformAnalysisGreedyMP.hh>
#include <algorithm>
#include <cmath>
#include <limits>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

void WaveformAnalysisGreedyMP::Configure(const std::string& config_name) {
  debug << GetAnalyzerName() << ": Configure called with config_name " << config_name << newline;
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);

    // ROI
    process_threshold_crossing = (fDigit->GetI("process_threshold_crossing") != 0);
    voltage_threshold = fDigit->GetD("voltage_threshold");
    threshold_region_padding = fDigit->GetI("region_padding");

    // Single-PE response template: set 1 (gaussian) when the detector SER is
    // gaussian, e.g. the Eos PMTPULSE tables.
    template_type = fDigit->GetI("template_type");
    if (template_type == 0) {
      lognormal_scale = fDigit->GetD("lognormal_scale");
      lognormal_shape = fDigit->GetD("lognormal_shape");
    } else if (template_type == 1) {
      gaussian_width = fDigit->GetD("gaussian_width");
      // Optional per-PMT-type template widths (paired arrays); unlisted PMT
      // types fall back to gaussian_width.
      gaussian_width_types.clear();
      gaussian_width_values.clear();
      try {
        gaussian_width_types = fDigit->GetIArray("gaussian_width_pmt_types");
        gaussian_width_values = fDigit->GetDArray("gaussian_width_pmt_widths");
      } catch (DBNotFoundError&) {
      }
      if (gaussian_width_types.size() != gaussian_width_values.size()) {
        RAT::Log::Die(GetAnalyzerName() + ": gaussian_width_pmt_types/widths must have equal length.");
      }
    } else {
      RAT::Log::Die(GetAnalyzerName() + ": Invalid template_type " + std::to_string(template_type) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }
    vpe_charge = fDigit->GetD("vpe_charge");

    // Dictionary
    upsample_factor = fDigit->GetD("upsampling_factor");

    // Noise / charge prior
    noise_sigma = fDigit->GetD("noise_sigma");
    gamma_k = fDigit->GetD("gamma_k");
    gamma_theta = fDigit->GetD("gamma_theta");

    // Model search
    seed_analyzer = fDigit->GetS("seed_analyzer");
    seed_missing_warned = false;
    max_iterations = fDigit->GetI("max_iterations");
    greedy_shortlist = fDigit->GetI("greedy_shortlist");

    // NPE estimation. Splitting a resolved atom's charge into integer PEs
    // reintroduces the single PE charge variance that matching pursuit removes
    // by resolving PEs individually, so both of these default off.
    npe_estimate = fDigit->GetZ("npe_estimate");
    npe_estimate_charge_width = fDigit->GetD("npe_estimate_charge_width");
    npe_estimate_max_pes = fDigit->GetI("npe_estimate_max_pes");
    weight_merge_window = fDigit->GetD("weight_merge_window");

    if (upsample_factor <= 0) {
      RAT::Log::Die(GetAnalyzerName() + ": Invalid upsampling factor.");
    }

    fWCache.clear();
    cached_nsamples = -1;
    cached_digitizer_period = -1.0;

  } catch (DBNotFoundError&) {
    RAT::Log::Die(GetAnalyzerName() + ": Unable to find analysis parameters.");
  }
}

void WaveformAnalysisGreedyMP::SetD(std::string param, double value) {
  if (param == "lognormal_scale") {
    lognormal_scale = value;
  } else if (param == "lognormal_shape") {
    lognormal_shape = value;
  } else if (param == "gaussian_width") {
    gaussian_width = value;
    fWCache.clear();
  } else if (param == "vpe_charge") {
    vpe_charge = value;
  } else if (param == "upsampling_factor") {
    upsample_factor = value;
    fWCache.clear();
  } else if (param == "voltage_threshold") {
    voltage_threshold = value;
  } else if (param == "noise_sigma") {
    noise_sigma = value;
  } else if (param == "gamma_k") {
    gamma_k = value;
  } else if (param == "gamma_theta") {
    gamma_theta = value;
  } else if (param == "npe_estimate_charge_width") {
    npe_estimate_charge_width = value;
  } else if (param == "weight_merge_window") {
    weight_merge_window = value;
  } else {
    WaveformAnalyzerBase::SetD(param, value);
  }
}

void WaveformAnalysisGreedyMP::SetI(std::string param, int value) {
  if (param == "process_threshold_crossing") {
    process_threshold_crossing = (value != 0);
  } else if (param == "template_type") {
    if (value != 0 && value != 1) {
      RAT::Log::Die(GetAnalyzerName() + ": Invalid template_type " + std::to_string(value) +
                    ". Must be 0 (lognormal) or 1 (gaussian).");
    }
    template_type = value;
    fWCache.clear();
  } else if (param == "region_padding") {
    threshold_region_padding = value;
  } else if (param == "max_iterations") {
    max_iterations = static_cast<size_t>(value);
  } else if (param == "greedy_shortlist") {
    greedy_shortlist = value;
  } else if (param == "npe_estimate") {
    npe_estimate = (value != 0);
  } else if (param == "npe_estimate_max_pes") {
    npe_estimate_max_pes = static_cast<size_t>(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisGreedyMP::SetS(std::string param, std::string value) {
  if (param == "seed_analyzer") {
    seed_analyzer = value;
    seed_missing_warned = false;
  } else {
    WaveformAnalyzerBase::SetS(param, value);
  }
}

TMatrixD WaveformAnalysisGreedyMP::BuildActive(const TMatrixD& W, int nrows, const std::vector<int>& cols) {
  TMatrixD A(nrows, static_cast<int>(cols.size()));
  for (size_t j = 0; j < cols.size(); ++j) {
    for (int i = 0; i < nrows; ++i) A(i, static_cast<int>(j)) = W(i, cols[j]);
  }
  return A;
}

void WaveformAnalysisGreedyMP::BuildDictionaryMatrix(int nsamples, double digitizer_period, double width,
                                                     TMatrixD& W_out) {
  debug << GetAnalyzerName() << ": Building dictionary matrix " << nsamples << " x "
        << static_cast<int>(nsamples * upsample_factor) << newline;

  const int dict_size = static_cast<int>(nsamples * upsample_factor);
  W_out.ResizeTo(nsamples, dict_size);
  W_out.Zero();

  // mag_factor maps the (unit-integral) lognormal time PDF [1/ns] to a voltage
  // [mV] such that a column with weight 1 corresponds to a PE of vpe_charge.
  // charge[pC] = -V[mV] * dt[ns] / Ohm  =>  integral(template)*dt = vpe_charge.
  const double mag_factor = vpe_charge * fTermOhms;

  for (int col = 0; col < dict_size; ++col) {
    const double delay = col * digitizer_period / upsample_factor;
    const double lognormal_shift = delay - lognormal_scale;
    for (int row = 0; row < nsamples; ++row) {
      const double sample_time = row * digitizer_period;
      double template_val = 0.0;
      if (template_type == 0) {  // lognormal
        if (sample_time > lognormal_shift) {
          template_val = mag_factor * TMath::LogNormal(sample_time, lognormal_shape, lognormal_shift, lognormal_scale);
        }
      } else {  // gaussian
        template_val = mag_factor * TMath::Gaus(sample_time, delay, width, kTRUE);
      }
      // Pulses are negative-going, matching the (pedestal-subtracted) voltage waveform.
      W_out(row, col) = -template_val;
    }
  }
}

double WaveformAnalysisGreedyMP::LogEvidence(const TMatrixD& W_active, const TVectorD& voltVec, TVectorD& charges_out) {
  const int D = voltVec.GetNrows();
  const int K = W_active.GetNcols();

  const double noise_var = noise_sigma * noise_sigma;
  // Gamma charge prior (in nominal-PE weight units): mean = k*theta, var = k*theta^2.
  const double charge_mean = gamma_k * gamma_theta;
  const double charge_var = gamma_k * gamma_theta * gamma_theta;
  const double neg_inf = -std::numeric_limits<double>::infinity();

  const double log2pi = std::log(2.0 * TMath::Pi());

  if (K == 0) {
    charges_out.ResizeTo(0);
    const double rTr = voltVec * voltVec;
    return -0.5 * (D * log2pi + D * std::log(noise_var) + rTr / noise_var);
  }

  // C = W^T W + (noise_var / charge_var) I   (K x K, symmetric positive definite)
  const double ridge = noise_var / charge_var;
  TMatrixD Wt(TMatrixD::kTransposed, W_active);
  TMatrixD C(W_active, TMatrixD::kTransposeMult, W_active);  // W^T W
  for (int i = 0; i < K; ++i) C(i, i) += ridge;

  TDecompChol chol(C);
  if (!chol.Decompose()) {
    charges_out.ResizeTo(K);
    charges_out.Zero();
    return neg_inf;
  }

  // log|C| = 2 * sum(log(diag(U))), with C = U^T U
  const TMatrixD& U = chol.GetU();
  double logdetC = 0.0;
  for (int i = 0; i < K; ++i) logdetC += std::log(U(i, i));
  logdetC *= 2.0;

  // Prior mean vector m = charge_mean * 1
  TVectorD m(K);
  for (int i = 0; i < K; ++i) m(i) = charge_mean;

  // r = v - W m ;  Atr = W^T r ;  Atv = W^T v
  TVectorD Wm = W_active * m;
  TVectorD r = voltVec - Wm;
  TVectorD Atr = Wt * r;
  TVectorD Atv = Wt * voltVec;

  // Reduced quadratic term: r^T Sigma^-1 r = (r^T r - Atr^T C^-1 Atr) / noise_var
  TVectorD y = Atr;
  chol.Solve(y);  // y = C^-1 Atr
  const double quad_reduced = Atr * y;
  const double rTr = r * r;
  const double quad = (rTr - quad_reduced) / noise_var;

  // log|Sigma| = D log(noise_var) + K log(charge_var/noise_var) + log|C|
  const double logdetSigma = D * std::log(noise_var) + K * std::log(charge_var / noise_var) + logdetC;

  const double logp = -0.5 * (D * log2pi + logdetSigma + quad);

  // Posterior mean charges: q_hat = C^-1 (W^T v + ridge * m)
  TVectorD rhs = Atv;
  rhs += ridge * m;
  chol.Solve(rhs);
  charges_out.ResizeTo(K);
  charges_out = rhs;

  return logp;
}

std::vector<std::pair<int, int>> WaveformAnalysisGreedyMP::FindThresholdRegions(const std::vector<double>& voltWfm,
                                                                                double threshold, int region_padding) {
  std::vector<std::pair<int, int>> regions;
  bool in_region = false;
  int region_start = -1;

  for (size_t i = 0; i < voltWfm.size(); ++i) {
    if (voltWfm[i] < threshold && !in_region) {
      region_start = std::max(0, static_cast<int>(i) - region_padding);
      in_region = true;
    } else if (voltWfm[i] >= threshold && in_region) {
      int region_end = std::min(static_cast<int>(voltWfm.size()) - 1, static_cast<int>(i) + region_padding - 1);
      regions.emplace_back(region_start, region_end);
      in_region = false;
    }
  }
  if (in_region) {
    regions.emplace_back(region_start, static_cast<int>(voltWfm.size()) - 1);
  }

  // Merge close/overlapping regions
  if (regions.size() > 1) {
    std::vector<std::pair<int, int>> merged;
    merged.push_back(regions[0]);
    for (size_t i = 1; i < regions.size(); ++i) {
      if (regions[i].first <= merged.back().second + region_padding) {
        merged.back().second = std::max(merged.back().second, regions[i].second);
      } else {
        merged.push_back(regions[i]);
      }
    }
    std::swap(regions, merged);
  }
  return regions;
}

bool WaveformAnalysisGreedyMP::PrepareRegion(const TMatrixD& fW, const std::vector<double>& voltWfm, int start_sample,
                                             int end_sample, Region& region_out) {
  const int region_length = end_sample - start_sample + 1;
  if (region_length <= 0) return false;

  // Candidate dictionary columns whose PE time falls within the region.
  const int dict_start = std::max(0, static_cast<int>(start_sample * upsample_factor));
  const int dict_end = std::min(fW.GetNcols() - 1, static_cast<int>(end_sample * upsample_factor));
  const int dict_cols = dict_end - dict_start + 1;
  if (dict_cols <= 0) return false;

  region_out.start_sample = start_sample;
  region_out.end_sample = end_sample;
  region_out.dict_start = dict_start;
  region_out.dict_cols = dict_cols;

  region_out.v.ResizeTo(region_length);
  for (int i = 0; i < region_length; ++i) region_out.v(i) = voltWfm[start_sample + i];

  region_out.W.ResizeTo(region_length, dict_cols);
  region_out.W.Zero();
  for (int row = 0; row < region_length; ++row) {
    const int global_row = start_sample + row;
    for (int col = 0; col < dict_cols; ++col) {
      region_out.W(row, col) = fW(global_row, dict_start + col);
    }
  }

  region_out.active.clear();
  region_out.charges.ResizeTo(0);
  region_out.logev = 0.0;
  return true;
}

size_t WaveformAnalysisGreedyMP::SeedRegions(DS::DigitPMT* digitpmt, std::vector<Region>& regions) {
  const std::vector<std::string> fitters = digitpmt->GetFitterNames();
  if (std::find(fitters.begin(), fitters.end(), seed_analyzer) == fitters.end()) {
    if (!seed_missing_warned) {
      warn << GetAnalyzerName() << ": no result named \"" << seed_analyzer
           << "\" to seed from. Does that analyzer run first in the macro? "
           << "Falling back to greedy selection." << newline;
      seed_missing_warned = true;
    }
    return 0;
  }
  DS::WaveformAnalysisResult* seed = digitpmt->GetOrCreateWaveformAnalysisResult(seed_analyzer);
  const std::vector<Double_t>& seed_times = seed->getTimes();
  const std::vector<Double_t>& seed_charges = seed->getCharges();

  const double dtp = fTimeStep / upsample_factor;
  // AddPE() stored those times with the cable offset already subtracted, while
  // the dictionary is in raw digitizer time, so put the offset back.
  const double time_offset = digitpmt->GetTimeOffset();

  // Collect (column, charge) per region. A grid point hosts at most one PE, so
  // duplicate columns collapse -- which is also what folds together the
  // repeated times an analyzer emits when it splits a hit into integer PEs.
  std::vector<std::vector<std::pair<int, double>>> picks(regions.size());
  for (size_t i = 0; i < seed_times.size(); ++i) {
    if (seed_times[i] == WaveformUtil::INVALID) continue;
    const int g = static_cast<int>(std::lround((seed_times[i] + time_offset) / dtp));
    const double q = (i < seed_charges.size()) ? seed_charges[i] : 0.0;
    for (size_t r = 0; r < regions.size(); ++r) {
      const int col = g - regions[r].dict_start;
      if (col < 0 || col >= regions[r].dict_cols) continue;
      auto& pr = picks[r];
      auto it = std::find_if(pr.begin(), pr.end(), [col](const std::pair<int, double>& e) { return e.first == col; });
      if (it == pr.end()) {
        pr.emplace_back(col, q);
      } else {
        it->second += q;
      }
      break;
    }
  }

  size_t nseed = 0;
  for (size_t r = 0; r < regions.size(); ++r) {
    auto& pr = picks[r];
    if (pr.empty()) continue;
    // Keep the largest max_iterations atoms, so a runaway seed cannot blow up
    // the O(K^3) evidence solve.
    if (pr.size() > max_iterations) {
      std::partial_sort(
          pr.begin(), pr.begin() + max_iterations, pr.end(),
          [](const std::pair<int, double>& a, const std::pair<int, double>& b) { return a.second > b.second; });
      pr.resize(max_iterations);
    }
    Region& reg = regions[r];
    reg.active.clear();
    for (const auto& e : pr) reg.active.push_back(e.first);
    std::sort(reg.active.begin(), reg.active.end());
    reg.logev = LogEvidence(BuildActive(reg.W, reg.v.GetNrows(), reg.active), reg.v, reg.charges);
    if (!std::isfinite(reg.logev)) {  // singular fit, let the greedy step retry
      reg.active.clear();
      reg.charges.ResizeTo(0);
      reg.logev = 0.0;
      continue;
    }
    nseed += reg.active.size();
  }
  return nseed;
}

void WaveformAnalysisGreedyMP::EmitRegion(const Region& region, DS::WaveformAnalysisResult* fit_result,
                                          double gain_calibration, double chi2ndf) {
  // Collect (time, weight) pairs of the occupied columns.
  std::vector<std::pair<double, double>> pes;  // (time, weight)
  pes.reserve(region.active.size());
  for (size_t j = 0; j < region.active.size(); ++j) {
    const double weight = region.charges(static_cast<int>(j));
    if (weight <= 0.0) continue;
    const double time = (region.dict_start + region.active[j]) * fTimeStep / upsample_factor;
    pes.emplace_back(time, weight);
  }
  if (pes.empty()) return;

  // Optional dominant-atom merge: fold atoms within weight_merge_window of the
  // largest remaining atom into it, at their charge-weighted mean time.
  if (weight_merge_window > 0.0 && pes.size() > 1) {
    std::vector<size_t> order(pes.size());
    for (size_t j = 0; j < order.size(); ++j) order[j] = j;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return pes[a].second > pes[b].second; });
    std::vector<char> assigned(pes.size(), 0);
    std::vector<std::pair<double, double>> merged;
    merged.reserve(pes.size());
    for (size_t seed : order) {
      if (assigned[seed]) continue;
      const double seed_time = pes[seed].first;
      double wsum = 0.0, twsum = 0.0;
      for (size_t j = 0; j < pes.size(); ++j) {
        if (assigned[j]) continue;
        if (std::abs(pes[j].first - seed_time) <= weight_merge_window) {
          assigned[j] = 1;
          wsum += pes[j].second;
          twsum += pes[j].first * pes[j].second;
        }
      }
      merged.emplace_back(twsum / wsum, wsum);
    }
    std::sort(merged.begin(), merged.end());
    pes.swap(merged);
  }

  // One PE per resolved atom, unless npe_estimate splits it by charge.
  const double calibrated_vpe_charge = vpe_charge * gain_calibration;
  for (const auto& [time, weight] : pes) {
    const double pe_charge = weight * vpe_charge * gain_calibration;

    size_t npe = npe_estimate
                     ? EstimateNPE(pe_charge, calibrated_vpe_charge, npe_estimate_charge_width, npe_estimate_max_pes)
                     : 1;
    if (npe == 0) npe = 1;

    for (size_t ipe = 0; ipe < npe; ++ipe) {
      std::map<std::string, double> fom;
      fom["chi2ndf"] = chi2ndf;
      fom["estimated_npe"] = static_cast<double>(npe);
      fit_result->AddPE(time, pe_charge / npe, fom);
    }
  }
}

void WaveformAnalysisGreedyMP::GreedySelect(Region& region, double logodds) {
  const TMatrixD& W = region.W;
  const TVectorD& v = region.v;
  const int region_length = v.GetNrows();
  const int dict_cols = region.dict_cols;

  region.active.clear();
  region.charges.ResizeTo(0);

  std::vector<char> inActive(dict_cols, 0);
  TVectorD dummy;
  double current_logev = LogEvidence(TMatrixD(region_length, 0), v, dummy);  // empty-set evidence
  double current_score = current_logev;
  region.logev = current_logev;

  // Matching-pursuit state: the next atom is picked by its correlation with the
  // running residual, so only greedy_shortlist candidates per step need the
  // expensive LogEvidence solve.
  TVectorD residual = v;
  const int shortlist = std::max(1, greedy_shortlist);

  while (region.active.size() < max_iterations) {
    // Correlate the dictionary with the current residual and shortlist the
    // highest-|correlation| inactive columns.
    std::vector<std::pair<double, int>> corr;  // (-|corr|, col) so the smallest sort first
    corr.reserve(dict_cols);
    for (int c = 0; c < dict_cols; ++c) {
      if (inActive[c]) continue;
      double dot = 0.0;
      for (int i = 0; i < region_length; ++i) dot += W(i, c) * residual(i);
      corr.emplace_back(-std::fabs(dot), c);
    }
    if (corr.empty()) break;
    const int ntop = std::min<int>(shortlist, static_cast<int>(corr.size()));
    std::partial_sort(corr.begin(), corr.begin() + ntop, corr.end());

    // Among the shortlist, keep the atom that best improves the penalized
    // evidence (evidence gain minus the per-PE occupancy penalty -logodds).
    double best_score = current_score;
    double best_logev = current_logev;
    int best_col = -1;
    TVectorD best_charges;
    for (int t = 0; t < ntop; ++t) {
      const int c = corr[t].second;
      std::vector<int> trial = region.active;
      trial.push_back(c);
      std::sort(trial.begin(), trial.end());
      TVectorD trial_charges;
      double logp = LogEvidence(BuildActive(W, region_length, trial), v, trial_charges);
      if (!std::isfinite(logp)) continue;
      double score = logp + static_cast<double>(trial.size()) * logodds;
      if (score > best_score) {
        best_score = score;
        best_logev = logp;
        best_col = c;
        best_charges.ResizeTo(trial_charges);
        best_charges = trial_charges;
      }
    }

    if (best_col < 0) break;  // no shortlisted atom improves the posterior

    region.active.push_back(best_col);
    inActive[best_col] = 1;
    std::sort(region.active.begin(), region.active.end());
    current_score = best_score;
    current_logev = best_logev;
    region.logev = best_logev;
    region.charges.ResizeTo(best_charges);
    region.charges = best_charges;

    // Refit residual with the updated active set (charges already correspond to
    // the sorted active order, so charges[j] matches active[j]).
    TMatrixD A = BuildActive(W, region_length, region.active);
    TVectorD fitted = A * region.charges;
    for (int i = 0; i < region_length; ++i) residual(i) = v(i) - fitted(i);
  }
}

void WaveformAnalysisGreedyMP::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  // Invalidate the dictionary cache when digitizer parameters change
  const double period_tolerance = 1e-9;
  if (cached_nsamples != static_cast<int>(digitWfm.size()) ||
      std::abs(cached_digitizer_period - fTimeStep) > period_tolerance) {
    fWCache.clear();
    cached_nsamples = static_cast<int>(digitWfm.size());
    cached_digitizer_period = fTimeStep;
  }

  // Pick the template width for this PMT's type (gaussian template only) and
  // fetch/build the matching dictionary.
  double width = gaussian_width;
  if (template_type == 1 && !gaussian_width_types.empty()) {
    const int pmt_type = DS::RunStore::GetCurrentRun()->GetPMTInfo()->GetType(digitpmt->GetID());
    for (size_t i = 0; i < gaussian_width_types.size(); ++i) {
      if (gaussian_width_types[i] == pmt_type) {
        width = gaussian_width_values[i];
        break;
      }
    }
  }
  const int cache_key = (template_type == 0) ? -1 : static_cast<int>(std::lround(width * 1000.0));
  auto cache_it = fWCache.find(cache_key);
  if (cache_it == fWCache.end()) {
    cache_it = fWCache.emplace(cache_key, TMatrixD()).first;
    BuildDictionaryMatrix(cached_nsamples, cached_digitizer_period, width, cache_it->second);
  }
  const TMatrixD& fW = cache_it->second;

  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die(GetAnalyzerName() + ": Pedestal is invalid! Did you run WaveformPrep first?");
  }

  if (noise_sigma <= 0.0) {
    RAT::Log::Die(GetAnalyzerName() + ": noise_sigma must be > 0.");
  }

  double gain_calibration = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetChargeScaleByPMTID(digitpmt->GetID());

  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  // Preconditioner: educated guess of the PE count from the integrated charge
  // computed upstream by WaveformPrep.
  double mu0 = digitpmt->GetDigitizedTotalCharge() / (vpe_charge * gain_calibration);
  if (!std::isfinite(mu0) || mu0 < 0.0) mu0 = 0.0;

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult(GetAnalyzerName());

  // ---- carve the waveform into regions of interest ----
  std::vector<Region> regions;
  if (process_threshold_crossing) {
    for (const auto& bounds : FindThresholdRegions(voltWfm, voltage_threshold, threshold_region_padding)) {
      Region region;
      if (PrepareRegion(fW, voltWfm, bounds.first, bounds.second, region)) regions.push_back(std::move(region));
    }
  } else {
    Region region;
    if (PrepareRegion(fW, voltWfm, 0, static_cast<int>(voltWfm.size()) - 1, region))
      regions.push_back(std::move(region));
  }
  if (regions.empty()) return;

  // Initial PE configuration: either another analyzer's result or, by default,
  // greedy forward selection. A seeded PE count is the better preconditioner
  // intensity, since it comes from an algorithm that actually resolves PEs
  // rather than from the integrated charge.
  size_t nseed = 0;
  if (!seed_analyzer.empty()) nseed = SeedRegions(digitpmt, regions);
  if (nseed > 0) mu0 = static_cast<double>(nseed);

  // Sparsity penalty for the greedy step. The per-grid-point occupancy is
  // normalised over the columns a PE can actually occupy, i.e. the union of the
  // regions; normalising over the full-waveform dictionary instead would make
  // logodds too negative by log(N_full / N_roi) per PE and under-select.
  int total_cols = 0;
  for (const Region& region : regions) total_cols += region.dict_cols;
  double p0 = (total_cols > 0) ? mu0 / total_cols : 0.0;
  p0 = std::min(0.5, std::max(1e-4, p0));
  const double logodds = std::log(p0 / (1.0 - p0));

  for (Region& region : regions) {
    if (region.active.empty()) GreedySelect(region, logodds);
  }

  // Goodness of fit over the whole waveform, so that every PE of this waveform
  // carries the same figure of merit.
  double rss = 0.0;
  int npoints = 0, nparams = 0;
  for (const Region& region : regions) {
    const int region_length = region.v.GetNrows();
    npoints += region_length;
    nparams += static_cast<int>(region.active.size());
    if (region.active.empty()) {
      for (int i = 0; i < region_length; ++i) rss += region.v(i) * region.v(i);
      continue;
    }
    TMatrixD A = BuildActive(region.W, region_length, region.active);
    TVectorD fitted = A * region.charges;
    for (int i = 0; i < region_length; ++i) {
      const double res = region.v(i) - fitted(i);
      rss += res * res;
    }
  }
  const int dof = std::max(1, npoints - nparams);
  const double chi2ndf = rss / (noise_sigma * noise_sigma * dof);

  for (const Region& region : regions) {
    EmitRegion(region, fit_result, gain_calibration, chi2ndf);
  }
}

}  // namespace RAT
