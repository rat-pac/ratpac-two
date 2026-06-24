#include <TDecompChol.h>
#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/NPEEstimator.hh>
#include <RAT/WaveformAnalysisFSMP.hh>
#include <algorithm>
#include <cmath>
#include <limits>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

void WaveformAnalysisFSMP::Configure(const std::string& config_name) {
  debug << "WaveformAnalysisFSMP: Configure called with config_name " << config_name << newline;
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);

    // ROI / preconditioner
    process_threshold_crossing = (fDigit->GetI("process_threshold_crossing") != 0);
    voltage_threshold = fDigit->GetD("voltage_threshold");
    threshold_region_padding = fDigit->GetI("region_padding");

    // Single-PE response (lognormal)
    lognormal_scale = fDigit->GetD("lognormal_scale");
    lognormal_shape = fDigit->GetD("lognormal_shape");
    vpe_charge = fDigit->GetD("vpe_charge");

    // Dictionary
    upsample_factor = fDigit->GetD("upsampling_factor");

    // Noise / charge prior
    noise_sigma = fDigit->GetD("noise_sigma");
    gamma_k = fDigit->GetD("gamma_k");
    gamma_theta = fDigit->GetD("gamma_theta");

    // Model search
    max_iterations = fDigit->GetI("max_iterations");
    enable_stochastic = fDigit->GetZ("enable_stochastic");
    n_mcmc_samples = fDigit->GetI("n_mcmc_samples");
    burn_in = fDigit->GetI("burn_in");
    random_seed = fDigit->GetI("random_seed");

    // NPE estimation
    npe_estimate = fDigit->GetZ("npe_estimate");
    npe_estimate_charge_width = fDigit->GetD("npe_estimate_charge_width");
    npe_estimate_max_pes = fDigit->GetI("npe_estimate_max_pes");

    if (upsample_factor <= 0) {
      RAT::Log::Die("WaveformAnalysisFSMP: Invalid upsampling factor.");
    }

    dictionary_built = false;
    cached_nsamples = -1;
    cached_digitizer_period = -1.0;

  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisFSMP: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisFSMP::SetD(std::string param, double value) {
  if (param == "lognormal_scale") {
    lognormal_scale = value;
  } else if (param == "lognormal_shape") {
    lognormal_shape = value;
  } else if (param == "vpe_charge") {
    vpe_charge = value;
  } else if (param == "upsampling_factor") {
    upsample_factor = value;
    dictionary_built = false;
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
  } else {
    WaveformAnalyzerBase::SetD(param, value);
  }
}

void WaveformAnalysisFSMP::SetI(std::string param, int value) {
  if (param == "process_threshold_crossing") {
    process_threshold_crossing = (value != 0);
  } else if (param == "region_padding") {
    threshold_region_padding = value;
  } else if (param == "max_iterations") {
    max_iterations = static_cast<size_t>(value);
  } else if (param == "enable_stochastic") {
    enable_stochastic = (value != 0);
  } else if (param == "n_mcmc_samples") {
    n_mcmc_samples = static_cast<size_t>(value);
  } else if (param == "burn_in") {
    burn_in = static_cast<size_t>(value);
  } else if (param == "random_seed") {
    random_seed = value;
  } else if (param == "npe_estimate") {
    npe_estimate = (value != 0);
  } else if (param == "npe_estimate_max_pes") {
    npe_estimate_max_pes = static_cast<size_t>(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisFSMP::BuildDictionaryMatrix(int nsamples, double digitizer_period) {
  debug << "WaveformAnalysisFSMP: Building dictionary matrix " << nsamples << " x "
        << static_cast<int>(nsamples * upsample_factor) << newline;

  const int dict_size = static_cast<int>(nsamples * upsample_factor);
  fW.ResizeTo(nsamples, dict_size);
  fW.Zero();

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
      if (sample_time > lognormal_shift) {
        template_val = mag_factor * TMath::LogNormal(sample_time, lognormal_shape, lognormal_shift, lognormal_scale);
      }
      // Pulses are negative-going, matching the (pedestal-subtracted) voltage waveform.
      fW(row, col) = -template_val;
    }
  }
}

double WaveformAnalysisFSMP::LogEvidence(const TMatrixD& W_active, const TVectorD& voltVec, TVectorD& charges_out) {
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

std::vector<std::pair<int, int>> WaveformAnalysisFSMP::FindThresholdRegions(const std::vector<double>& voltWfm,
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

void WaveformAnalysisFSMP::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  const double period_tolerance = 1e-9;
  if (!dictionary_built || cached_nsamples != static_cast<int>(digitWfm.size()) ||
      std::abs(cached_digitizer_period - fTimeStep) > period_tolerance) {
    BuildDictionaryMatrix(static_cast<int>(digitWfm.size()), fTimeStep);
    cached_nsamples = static_cast<int>(digitWfm.size());
    cached_digitizer_period = fTimeStep;
    dictionary_built = true;
  }

  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisFSMP: Pedestal is invalid! Did you run WaveformPrep first?");
  }

  if (noise_sigma <= 0.0) {
    RAT::Log::Die("WaveformAnalysisFSMP: noise_sigma must be > 0.");
  }

  double gain_calibration = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetChargeScaleByPMTID(digitpmt->GetID());

  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  // Preconditioner (fast, non-iterative): educated guess of the PE count from
  // the integrated charge computed upstream by WaveformPrep, converted into a
  // per-grid-point occupancy prior used as the sparsity penalty in the model
  // search (paper sec 3.5, h(z,t0) ~ p(z|mu0,t0)).
  double mu0 = digitpmt->GetDigitizedTotalCharge() / (vpe_charge * gain_calibration);
  if (!std::isfinite(mu0) || mu0 < 0.0) mu0 = 0.0;
  const double n_grid = static_cast<double>(fW.GetNcols());
  double p0 = (n_grid > 0.0) ? (mu0 / n_grid) : 0.0;
  p0 = std::min(0.5, std::max(1e-4, p0));
  const double logodds = std::log(p0 / (1.0 - p0));

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult(GetAnalyzerName());

  if (process_threshold_crossing) {
    std::vector<std::pair<int, int>> regions =
        FindThresholdRegions(voltWfm, voltage_threshold, threshold_region_padding);
    for (const auto& region : regions) {
      ProcessRegion(voltWfm, region.first, region.second, fit_result, gain_calibration, logodds);
    }
  } else {
    ProcessRegion(voltWfm, 0, static_cast<int>(voltWfm.size()) - 1, fit_result, gain_calibration, logodds);
  }
}

void WaveformAnalysisFSMP::ProcessRegion(const std::vector<double>& voltWfm, int start_sample, int end_sample,
                                         DS::WaveformAnalysisResult* fit_result, double gain_calibration,
                                         double logodds) {
  const int region_length = end_sample - start_sample + 1;
  if (region_length <= 0) return;

  // Candidate dictionary columns whose PE time falls within the region.
  const int dict_start = std::max(0, static_cast<int>(start_sample * upsample_factor));
  const int dict_end = std::min(fW.GetNcols() - 1, static_cast<int>(end_sample * upsample_factor));
  const int dict_cols = dict_end - dict_start + 1;
  if (dict_cols <= 0) return;

  // Region waveform vector
  TVectorD v(region_length);
  for (int i = 0; i < region_length; ++i) v(i) = voltWfm[start_sample + i];

  // Region sub-dictionary W_roi (region_length x dict_cols)
  TMatrixD W_roi(region_length, dict_cols);
  W_roi.Zero();
  for (int row = 0; row < region_length; ++row) {
    const int global_row = start_sample + row;
    for (int col = 0; col < dict_cols; ++col) {
      W_roi(row, col) = fW(global_row, dict_start + col);
    }
  }

  // Helper to build the active sub-matrix from local column indices.
  auto buildActive = [&](const std::vector<int>& cols) {
    TMatrixD A(region_length, static_cast<int>(cols.size()));
    for (size_t j = 0; j < cols.size(); ++j) {
      for (int i = 0; i < region_length; ++i) A(i, static_cast<int>(j)) = W_roi(i, cols[j]);
    }
    return A;
  };

  // Greedy forward selection (deterministic FBMP, Phase 1).
  std::vector<int> active;
  TVectorD dummy;
  double current_score = LogEvidence(TMatrixD(region_length, 0), v, dummy);  // empty-set evidence
  TVectorD best_charges;

  while (active.size() < max_iterations) {
    double best_delta = 0.0;
    int best_col = -1;
    TVectorD best_trial_charges;

    for (int c = 0; c < dict_cols; ++c) {
      if (std::find(active.begin(), active.end(), c) != active.end()) continue;
      std::vector<int> trial = active;
      trial.push_back(c);
      TMatrixD A = buildActive(trial);
      TVectorD trial_charges;
      double logp = LogEvidence(A, v, trial_charges);
      if (!std::isfinite(logp)) continue;
      // Net change in log-posterior from adding one PE (evidence gain minus
      // Occam/occupancy penalty -logodds).
      double score = logp + static_cast<double>(trial.size()) * logodds;
      double delta = score - current_score;
      if (delta > best_delta) {
        best_delta = delta;
        best_col = c;
        best_trial_charges.ResizeTo(trial_charges);
        best_trial_charges = trial_charges;
      }
    }

    if (best_col < 0) break;  // no addition improves the posterior
    active.push_back(best_col);
    current_score += best_delta;
    best_charges.ResizeTo(best_trial_charges);
    best_charges = best_trial_charges;
  }

  if (active.empty()) return;

  // Reconstructed waveform for goodness-of-fit (chi2/ndf normalized by noise).
  TMatrixD A_final = buildActive(active);
  TVectorD fitted = A_final * best_charges;
  double rss = 0.0;
  for (int i = 0; i < region_length; ++i) {
    const double res = v(i) - fitted(i);
    rss += res * res;
  }
  const int dof = std::max(1, region_length - static_cast<int>(active.size()));
  const double chi2ndf = rss / (noise_sigma * noise_sigma * dof);

  // FSMP intensity estimator mu_hat = sum of charges (in PE units), eq. 3.4.
  double mu_hat = 0.0;
  for (int j = 0; j < best_charges.GetNrows(); ++j) mu_hat += std::max(0.0, best_charges(j));

  // Emit one PE per selected column (optionally split into integer PEs by charge).
  const double calibrated_vpe_charge = vpe_charge * gain_calibration;
  for (size_t j = 0; j < active.size(); ++j) {
    const double weight = best_charges(static_cast<int>(j));
    if (weight <= 0.0) continue;
    const int global_col = dict_start + active[j];
    const double time = global_col * fTimeStep / upsample_factor;
    const double pe_charge = weight * vpe_charge * gain_calibration;

    size_t npe = npe_estimate
                     ? EstimateNPE(pe_charge, calibrated_vpe_charge, npe_estimate_charge_width, npe_estimate_max_pes)
                     : 1;
    if (npe == 0) npe = 1;

    for (size_t ipe = 0; ipe < npe; ++ipe) {
      fit_result->AddPE(time, pe_charge / npe,
                        {
                            {"chi2ndf", chi2ndf},
                            {"fsmp_mu", mu_hat},
                            {"estimated_npe", static_cast<double>(npe)},
                        });
    }
  }
}

}  // namespace RAT
