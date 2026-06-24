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

    // Light curve prior on PE arrival times, used by the stochastic sampler.
    lightcurve_tau = fDigit->GetD("lightcurve_tau");
    lightcurve_sigma = fDigit->GetD("lightcurve_sigma");
    t0_step = fDigit->GetD("t0_step");
    fRNG = std::make_unique<TRandom3>(static_cast<UInt_t>(random_seed));

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
  } else if (param == "lightcurve_tau") {
    lightcurve_tau = value;
  } else if (param == "lightcurve_sigma") {
    lightcurve_sigma = value;
  } else if (param == "t0_step") {
    t0_step = value;
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
      ProcessRegion(voltWfm, region.first, region.second, fit_result, gain_calibration, logodds, mu0);
    }
  } else {
    ProcessRegion(voltWfm, 0, static_cast<int>(voltWfm.size()) - 1, fit_result, gain_calibration, logodds, mu0);
  }
}

double WaveformAnalysisFSMP::LightCurve(double dt) const {
  const double sigma = lightcurve_sigma;
  const double tau = lightcurve_tau;
  const double sqrt2 = std::sqrt(2.0);

  if (tau <= 1e-6) {
    // Pure Gaussian (Cherenkov limit, tau_l -> 0).
    if (sigma <= 0.0) return (std::abs(dt) < 1e-9) ? 1.0 : 0.0;
    return std::exp(-0.5 * (dt / sigma) * (dt / sigma)) / (sigma * std::sqrt(2.0 * TMath::Pi()));
  }
  if (sigma <= 0.0) {
    // Pure exponential (no timing spread).
    return (dt > 0.0) ? std::exp(-dt / tau) / tau : 0.0;
  }
  // Exponentially-modified Gaussian (ex-Gaussian), paper eq. 2.2.
  double expo = sigma * sigma / (2.0 * tau * tau) - dt / tau;
  expo = std::min(expo, 300.0);  // guard against overflow
  const double arg = sigma / (sqrt2 * tau) - dt / (sqrt2 * sigma);
  return (1.0 / (2.0 * tau)) * std::exp(expo) * TMath::Erfc(arg);
}

void WaveformAnalysisFSMP::GreedySelect(const TMatrixD& W_roi, const TVectorD& v, int dict_cols, double logodds,
                                        std::vector<int>& active, TVectorD& charges) {
  const int region_length = v.GetNrows();
  auto buildActive = [&](const std::vector<int>& cols) {
    TMatrixD A(region_length, static_cast<int>(cols.size()));
    for (size_t j = 0; j < cols.size(); ++j) {
      for (int i = 0; i < region_length; ++i) A(i, static_cast<int>(j)) = W_roi(i, cols[j]);
    }
    return A;
  };

  active.clear();
  TVectorD dummy;
  double current_score = LogEvidence(TMatrixD(region_length, 0), v, dummy);  // empty-set evidence

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
    charges.ResizeTo(best_trial_charges);
    charges = best_trial_charges;
  }
  std::sort(active.begin(), active.end());
  if (!active.empty()) {
    // Recompute charges for the sorted active set so charges[j] matches active[j].
    TMatrixD A = buildActive(active);
    LogEvidence(A, v, charges);
  }
}

void WaveformAnalysisFSMP::SampleConfigurations(const TMatrixD& W_roi, const TVectorD& v, int dict_cols, int dict_start,
                                                double mu0, std::vector<int>& active, TVectorD& charges, double& t0_hat,
                                                double& mu_hat) {
  const int region_length = v.GetNrows();
  const double dtp = fTimeStep / upsample_factor;  // grid spacing in ns
  const double mu_eff = std::max(mu0, 0.5);        // floor to keep prior non-degenerate

  auto colTime = [&](int c) { return (dict_start + c) * fTimeStep / upsample_factor; };
  auto buildActive = [&](const std::vector<int>& cols) {
    TMatrixD A(region_length, static_cast<int>(cols.size()));
    for (size_t j = 0; j < cols.size(); ++j) {
      for (int i = 0; i < region_length; ++i) A(i, static_cast<int>(j)) = W_roi(i, cols[j]);
    }
    return A;
  };
  // Per-column occupancy probability given t0 (paper h(z,t0) ~ p(z|mu0,t0)).
  auto pOcc = [&](int c, double t0) {
    double p = mu_eff * LightCurve(colTime(c) - t0) * dtp;
    return std::min(0.99, std::max(1e-6, p));
  };
  auto logPriorFull = [&](const std::vector<char>& inP, double t0) {
    double s = 0.0;
    for (int c = 0; c < dict_cols; ++c) {
      const double p = pOcc(c, t0);
      s += inP[c] ? std::log(p) : std::log(1.0 - p);
    }
    return s;
  };

  // Sampler state, initialised from the greedy result.
  std::vector<char> inP(dict_cols, 0);
  std::vector<int> P;
  for (int c : active) {
    if (c >= 0 && c < dict_cols && !inP[c]) {
      inP[c] = 1;
      P.push_back(c);
    }
  }
  std::sort(P.begin(), P.end());

  // t0 init: charge-weighted mean time of the initial PEs, else region center.
  double t0 = colTime(dict_cols / 2);
  if (!P.empty() && charges.GetNrows() == static_cast<int>(active.size())) {
    double wsum = 0.0, tsum = 0.0;
    for (size_t j = 0; j < active.size(); ++j) {
      const double w = std::max(0.0, charges(static_cast<int>(j)));
      wsum += w;
      tsum += w * colTime(active[j]);
    }
    if (wsum > 0.0) t0 = tsum / wsum;
  }
  const double t0_min = colTime(0) - 3.0 * std::max(lightcurve_sigma, 1.0);
  const double t0_max = colTime(dict_cols - 1) + 3.0 * std::max(lightcurve_sigma, 1.0);

  TVectorD cur_charges;
  double cur_logev = LogEvidence(buildActive(P), v, cur_charges);

  // MAP tracking and posterior accumulators.
  std::vector<int> best_P = P;
  TVectorD best_charges = cur_charges;
  double best_target = cur_logev + logPriorFull(inP, t0);
  double sum_t0 = 0.0, sum_mu = 0.0;
  size_t n_rec = 0;

  // Note: the common 1/3 move-type proposal factor cancels in the birth/death
  // acceptance ratio, so it is omitted below.
  const size_t total_iters = burn_in + n_mcmc_samples;

  for (size_t it = 0; it < total_iters; ++it) {
    // ---- z update: one birth/death/shift move ----
    const int npresent = static_cast<int>(P.size());
    const int nabsent = dict_cols - npresent;
    const double move = fRNG->Uniform();

    if (move < 1.0 / 3.0 && nabsent > 0) {
      // Birth: add a uniformly chosen inactive column.
      int c = -1, k = fRNG->Integer(nabsent), seen = 0;
      for (int j = 0; j < dict_cols; ++j) {
        if (!inP[j]) {
          if (seen == k) {
            c = j;
            break;
          }
          ++seen;
        }
      }
      std::vector<int> Pp = P;
      Pp.push_back(c);
      std::sort(Pp.begin(), Pp.end());
      TVectorD ch;
      double logev_p = LogEvidence(buildActive(Pp), v, ch);
      const double pc = pOcc(c, t0);
      const double dlogprior = std::log(pc) - std::log(1.0 - pc);
      const double logqratio = std::log(static_cast<double>(nabsent) / (npresent + 1));
      const double la = (logev_p - cur_logev) + dlogprior + logqratio;
      if (std::isfinite(logev_p) && std::log(fRNG->Uniform()) < la) {
        P.swap(Pp);
        inP[c] = 1;
        cur_logev = logev_p;
        cur_charges.ResizeTo(ch);
        cur_charges = ch;
      }
    } else if (move < 2.0 / 3.0 && npresent > 0) {
      // Death: remove a uniformly chosen active column.
      int idx = fRNG->Integer(npresent);
      int c = P[idx];
      std::vector<int> Pp = P;
      Pp.erase(Pp.begin() + idx);
      TVectorD ch;
      double logev_p = LogEvidence(buildActive(Pp), v, ch);
      const double pc = pOcc(c, t0);
      const double dlogprior = std::log(1.0 - pc) - std::log(pc);
      const double logqratio = std::log(static_cast<double>(npresent) / (dict_cols - npresent + 1));
      const double la = (logev_p - cur_logev) + dlogprior + logqratio;
      if (std::isfinite(logev_p) && std::log(fRNG->Uniform()) < la) {
        P.swap(Pp);
        inP[c] = 0;
        cur_logev = logev_p;
        cur_charges.ResizeTo(ch);
        cur_charges = ch;
      }
    } else if (npresent > 0) {
      // Shift: move a random active column to a free neighbor (symmetric proposal).
      int idx = fRNG->Integer(npresent);
      int c = P[idx];
      int dir = (fRNG->Uniform() < 0.5) ? -1 : 1;
      int cp = c + dir;
      if (cp >= 0 && cp < dict_cols && !inP[cp]) {
        std::vector<int> Pp = P;
        Pp[idx] = cp;
        std::sort(Pp.begin(), Pp.end());
        TVectorD ch;
        double logev_p = LogEvidence(buildActive(Pp), v, ch);
        const double pc = pOcc(c, t0), pcp = pOcc(cp, t0);
        const double dlogprior = (std::log(pcp) - std::log(1.0 - pcp)) + (std::log(1.0 - pc) - std::log(pc));
        const double la = (logev_p - cur_logev) + dlogprior;
        if (std::isfinite(logev_p) && std::log(fRNG->Uniform()) < la) {
          P.swap(Pp);
          inP[c] = 0;
          inP[cp] = 1;
          cur_logev = logev_p;
          cur_charges.ResizeTo(ch);
          cur_charges = ch;
        }
      }
    }

    // ---- t0 update: random-walk Metropolis (evidence independent of t0) ----
    const double t0_prop = t0 + fRNG->Gaus(0.0, t0_step);
    if (t0_prop >= t0_min && t0_prop <= t0_max) {
      const double dlp = logPriorFull(inP, t0_prop) - logPriorFull(inP, t0);
      if (std::log(fRNG->Uniform()) < dlp) t0 = t0_prop;
    }

    // ---- record post burn-in ----
    if (it >= burn_in) {
      double qsum = 0.0;
      for (int j = 0; j < cur_charges.GetNrows(); ++j) qsum += std::max(0.0, cur_charges(j));
      sum_t0 += t0;
      sum_mu += qsum;
      ++n_rec;
      const double target = cur_logev + logPriorFull(inP, t0);
      if (target > best_target) {
        best_target = target;
        best_P = P;
        best_charges.ResizeTo(cur_charges);
        best_charges = cur_charges;
      }
    }
  }

  t0_hat = (n_rec > 0) ? sum_t0 / n_rec : t0;
  mu_hat = (n_rec > 0) ? sum_mu / n_rec : 0.0;
  active = best_P;
  charges.ResizeTo(best_charges);
  charges = best_charges;
}

void WaveformAnalysisFSMP::ProcessRegion(const std::vector<double>& voltWfm, int start_sample, int end_sample,
                                         DS::WaveformAnalysisResult* fit_result, double gain_calibration,
                                         double logodds, double mu0) {
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

  auto buildActive = [&](const std::vector<int>& cols) {
    TMatrixD A(region_length, static_cast<int>(cols.size()));
    for (size_t j = 0; j < cols.size(); ++j) {
      for (int i = 0; i < region_length; ++i) A(i, static_cast<int>(j)) = W_roi(i, cols[j]);
    }
    return A;
  };

  // Greedy forward selection: the reported solution when the sampler is off,
  // and the initialisation for the sampler when it is on.
  std::vector<int> active;
  TVectorD charges;
  GreedySelect(W_roi, v, dict_cols, logodds, active, charges);

  // FSMP intensity estimator mu_hat = sum of charges (in PE units), eq. 3.4.
  double mu_hat = 0.0;
  for (int j = 0; j < charges.GetNrows(); ++j) mu_hat += std::max(0.0, charges(j));
  // t0 estimator: set by the sampler; otherwise a charge-weighted mean below.
  double t0_hat = WaveformUtil::INVALID;

  // Stochastic refinement (MH-within-Gibbs over z and t0).
  if (enable_stochastic) {
    SampleConfigurations(W_roi, v, dict_cols, dict_start, mu0, active, charges, t0_hat, mu_hat);
  }

  if (active.empty()) return;

  // Reconstructed waveform for goodness-of-fit (chi2/ndf normalized by noise).
  TMatrixD A_final = buildActive(active);
  TVectorD fitted = A_final * charges;
  double rss = 0.0;
  for (int i = 0; i < region_length; ++i) {
    const double res = v(i) - fitted(i);
    rss += res * res;
  }
  const int dof = std::max(1, region_length - static_cast<int>(active.size()));
  const double chi2ndf = rss / (noise_sigma * noise_sigma * dof);

  if (t0_hat == WaveformUtil::INVALID) {
    // Sampler off: report the charge-weighted mean reconstructed PE time.
    double wsum = 0.0, tsum = 0.0;
    for (size_t j = 0; j < active.size(); ++j) {
      const double w = std::max(0.0, charges(static_cast<int>(j)));
      wsum += w;
      tsum += w * (dict_start + active[j]) * fTimeStep / upsample_factor;
    }
    t0_hat = (wsum > 0.0) ? tsum / wsum : WaveformUtil::INVALID;
  }

  // Emit one PE per selected column (optionally split into integer PEs by charge).
  const double calibrated_vpe_charge = vpe_charge * gain_calibration;
  for (size_t j = 0; j < active.size(); ++j) {
    const double weight = charges(static_cast<int>(j));
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
                            {"fsmp_t0", t0_hat},
                            {"estimated_npe", static_cast<double>(npe)},
                        });
    }
  }
}

}  // namespace RAT
