#include <CLHEP/Random/Random.h>
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

    // Initial configuration
    seed_analyzer = fDigit->GetS("seed_analyzer");
    seed_missing_warned = false;
    max_iterations = fDigit->GetI("max_iterations");

    // Stochastic sampler
    n_mcmc_samples = fDigit->GetI("n_mcmc_samples");
    burn_in = fDigit->GetI("burn_in");

    // Light curve prior on PE arrival times, used by the sampler.
    lightcurve_tau = fDigit->GetD("lightcurve_tau");
    lightcurve_sigma = fDigit->GetD("lightcurve_sigma");
    t0_step = fDigit->GetD("t0_step");

    // NPE estimation. Splitting a resolved atom's charge into integer PEs
    // reintroduces the single PE charge variance that FSMP removes by counting
    // configurations, so both of these default off.
    npe_estimate = fDigit->GetZ("npe_estimate");
    npe_estimate_charge_width = fDigit->GetD("npe_estimate_charge_width");
    npe_estimate_max_pes = fDigit->GetI("npe_estimate_max_pes");
    weight_merge_window = fDigit->GetD("weight_merge_window");

    // Diagnostic output: one array of nsamples doubles per analyzed PMT per
    // event, so off unless asked for.
    store_expected_waveform = fDigit->GetZ("store_expected_waveform");

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

void WaveformAnalysisFSMP::BeginOfRun(DS::Run*) {
  // Seed from the global CLHEP engine so that `rat -s` controls the sampler as
  // it does every other random process. getTheSeed() reads the seed rather than
  // drawing from the stream, so enabling FSMP does not perturb the simulation.
  const long seed = CLHEP::HepRandom::getTheSeed();
  fRNG.SetSeed(static_cast<UInt_t>(seed == 0 ? 1 : seed));  // TRandom3 reads 0 as "seed from the clock"
}

void WaveformAnalysisFSMP::SetD(std::string param, double value) {
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
  } else if (param == "lightcurve_tau") {
    lightcurve_tau = value;
  } else if (param == "lightcurve_sigma") {
    lightcurve_sigma = value;
  } else if (param == "t0_step") {
    t0_step = value;
  } else if (param == "npe_estimate_charge_width") {
    npe_estimate_charge_width = value;
  } else if (param == "weight_merge_window") {
    weight_merge_window = value;
  } else {
    WaveformAnalyzerBase::SetD(param, value);
  }
}

void WaveformAnalysisFSMP::SetI(std::string param, int value) {
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
  } else if (param == "n_mcmc_samples") {
    n_mcmc_samples = static_cast<size_t>(value);
  } else if (param == "burn_in") {
    burn_in = static_cast<size_t>(value);
  } else if (param == "npe_estimate") {
    npe_estimate = (value != 0);
  } else if (param == "npe_estimate_max_pes") {
    npe_estimate_max_pes = static_cast<size_t>(value);
  } else if (param == "store_expected_waveform") {
    store_expected_waveform = (value != 0);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisFSMP::SetS(std::string param, std::string value) {
  if (param == "seed_analyzer") {
    seed_analyzer = value;
    seed_missing_warned = false;
  } else {
    WaveformAnalyzerBase::SetS(param, value);
  }
}

TMatrixD WaveformAnalysisFSMP::BuildActive(const TMatrixD& W, int nrows, const std::vector<int>& cols) {
  TMatrixD A(nrows, static_cast<int>(cols.size()));
  for (size_t j = 0; j < cols.size(); ++j) {
    for (int i = 0; i < nrows; ++i) A(i, static_cast<int>(j)) = W(i, cols[j]);
  }
  return A;
}

void WaveformAnalysisFSMP::BuildDictionaryMatrix(int nsamples, double digitizer_period, double width, TMatrixD& W_out) {
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

bool WaveformAnalysisFSMP::PrepareRegion(const TMatrixD& fW, const std::vector<double>& voltWfm, int start_sample,
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

size_t WaveformAnalysisFSMP::SeedRegions(DS::DigitPMT* digitpmt, std::vector<Region>& regions) {
  const std::vector<std::string> fitters = digitpmt->GetFitterNames();
  if (std::find(fitters.begin(), fitters.end(), seed_analyzer) == fitters.end()) {
    if (!seed_missing_warned) {
      warn << GetAnalyzerName() << ": no result named \"" << seed_analyzer
           << "\" to seed from. Does that analyzer run first in the macro? "
           << "Starting the chain from an empty configuration." << newline;
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
    if (!std::isfinite(reg.logev)) {  // singular fit, let the sampler start empty
      reg.active.clear();
      reg.charges.ResizeTo(0);
      reg.logev = 0.0;
      continue;
    }
    nseed += reg.active.size();
  }
  return nseed;
}

void WaveformAnalysisFSMP::EmitRegion(const Region& region, DS::WaveformAnalysisResult* fit_result,
                                      double gain_calibration, double chi2ndf,
                                      const std::map<std::string, double>& extra_foms) {
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
      std::map<std::string, double> fom = extra_foms;
      fom["chi2ndf"] = chi2ndf;
      fom["estimated_npe"] = static_cast<double>(npe);
      fit_result->AddPE(time, pe_charge / npe, fom);
    }
  }
}

std::vector<double> WaveformAnalysisFSMP::ExpectedWaveform(const std::vector<Region>& regions, const TMatrixD& fW,
                                                           int nsamples) const {
  std::vector<double> expected(nsamples, 0.0);
  for (const Region& region : regions) {
    // Regions were fit independently but their templates are not confined to
    // them, so sum the atoms over the full-window dictionary rather than the
    // region slice: that keeps the tail a PE leaves outside its own region.
    if (region.charges.GetNrows() != static_cast<int>(region.active.size())) continue;
    for (size_t j = 0; j < region.active.size(); ++j) {
      const double weight = region.charges(static_cast<int>(j));
      if (weight == 0.0) continue;
      const int gcol = region.dict_start + region.active[j];
      for (int row = 0; row < nsamples; ++row) expected[row] += weight * fW(row, gcol);
    }
  }
  return expected;
}

double WaveformAnalysisFSMP::IntensityMLE(const std::vector<size_t>& n_hist, double mu_ref, double mass_scale) const {
  // Paper eqs. 3.25-3.26: the chain draws z under mu_ref, so samples are
  // reweighted by p(z|mu,t0)/p(z|mu_ref,t0) before maximising over mu. With
  // sum_c p_c = mu * mass_scale that weight is (mu/mu_ref)^N exp(-(mu -
  // mu_ref) * mass_scale), so only the histogram of N is needed.
  size_t total = 0, nmax = 0;
  for (size_t n = 0; n < n_hist.size(); ++n) {
    if (n_hist[n] > 0) {
      total += n_hist[n];
      nmax = n;
    }
  }
  if (total == 0 || nmax == 0) return 0.0;

  const int ngrid = 1024;
  const double mu_max = std::max(5.0, 3.0 * (static_cast<double>(nmax) + 1.0));
  double best_mu = 0.0;
  double best_ll = -std::numeric_limits<double>::infinity();

  for (int i = 1; i <= ngrid; ++i) {
    const double mu = mu_max * i / ngrid;
    const double lr = std::log(mu / mu_ref);
    // log-sum-exp over the histogram of PE counts
    double peak = -std::numeric_limits<double>::infinity();
    for (size_t n = 0; n < n_hist.size(); ++n) {
      if (!n_hist[n]) continue;
      peak = std::max(peak, std::log(static_cast<double>(n_hist[n])) + n * lr);
    }
    double sum = 0.0;
    for (size_t n = 0; n < n_hist.size(); ++n) {
      if (!n_hist[n]) continue;
      sum += std::exp(std::log(static_cast<double>(n_hist[n])) + n * lr - peak);
    }
    const double ll = peak + std::log(sum) - (mu - mu_ref) * mass_scale;
    if (ll > best_ll) {
      best_ll = ll;
      best_mu = mu;
    }
  }
  return best_mu;
}

void WaveformAnalysisFSMP::SampleConfigurations(std::vector<Region>& regions, int dict_total, double mu0,
                                                double& t0_hat, double& mu_hat, double& npe_hat) {
  const double dtp = fTimeStep / upsample_factor;  // grid spacing in ns
  const double mu_eff = std::max(mu0, 0.5);        // floor to keep the prior non-degenerate

  // Flatten every region's candidate columns into one slot list: all regions are
  // sampled together so t0 and mu are per-waveform. The evidence stays
  // block-diagonal, so a move only re-solves the region it touches.
  struct Slot {
    int region;
    int col;
    int g;  // global dictionary column, i.e. time = g * dtp
  };
  std::vector<Slot> slots;
  std::vector<double> slot_w;  // birth proposal weight
  std::vector<int> region_base(regions.size(), 0);
  for (size_t r = 0; r < regions.size(); ++r) {
    if (r > 0) region_base[r] = region_base[r - 1] + regions[r - 1].dict_cols;
    const Region& reg = regions[r];
    const int D = reg.v.GetNrows();
    for (int c = 0; c < reg.dict_cols; ++c) {
      double dot = 0.0;
      for (int i = 0; i < D; ++i) dot += reg.W(i, c) * reg.v(i);
      slots.push_back({static_cast<int>(r), c, reg.dict_start + c});
      slot_w.push_back(std::fabs(dot));
    }
  }
  const int nslots = static_cast<int>(slots.size());
  if (nslots == 0) return;

  // Births are proposed proportional to each column's correlation with the
  // waveform, since a uniform proposal over this dense a grid rarely lands on
  // signal. The floor keeps every column reachable, and the weights are
  // state-independent so the Hastings ratios below stay exact.
  double wmax = 0.0;
  for (double w : slot_w) wmax = std::max(wmax, w);
  const double wfloor = (wmax > 0.0) ? 1e-3 * wmax : 1.0;
  double total_w = 0.0;
  for (double& w : slot_w) {
    w += wfloor;
    total_w += w;
  }

  // t0 is sampled on the same upsampled grid as the PE times (t0 = j * dtp), so
  // every phi(t_c - t0) falls on an integer multiple of dtp and tabulates once.
  int g_min = slots.front().g, g_max = slots.front().g;
  for (const Slot& s : slots) {
    g_min = std::min(g_min, s.g);
    g_max = std::max(g_max, s.g);
  }
  // PEs follow t0 for an ex-Gaussian light curve, so allow t0 well before the
  // first candidate column and only a few sigma after the last.
  const double pad_lo = 3.0 * std::max(lightcurve_sigma, 1.0) + std::max(lightcurve_tau, 0.0);
  const double pad_hi = 3.0 * std::max(lightcurve_sigma, 1.0);
  const int jlo = g_min - static_cast<int>(std::ceil(pad_lo / dtp));
  const int jhi = g_max + static_cast<int>(std::ceil(pad_hi / dtp));

  // The occupancy prior spans the whole digitizer window, not just the regions:
  // columns outside them are known to be empty. Summing over the regions alone
  // would drop a t0-dependent term and bias t0; renormalising phi over them
  // would let t0 run to its bound.
  const int m_off = -jhi;                             // smallest offset g - j that can occur
  const int nm = (dict_total - 1 - jlo) + 1 - m_off;  // tabulated offsets
  std::vector<double> phi_tab(nm);
  std::vector<double> log1mp_prefix(nm);  // prefix sums of log(1 - p) over offsets
  std::vector<double> phi_prefix(nm);     // prefix sums of phi, for the window mass
  double run_log = 0.0, run_phi = 0.0;
  for (int i = 0; i < nm; ++i) {
    const double phi = LightCurve((i + m_off) * dtp);
    phi_tab[i] = phi;
    const double p = std::min(0.99, std::max(1e-300, mu_eff * phi * dtp));
    run_log += std::log1p(-p);
    run_phi += phi * dtp;
    log1mp_prefix[i] = run_log;
    phi_prefix[i] = run_phi;
  }
  // Columns of the full dictionary run g = 0 .. dict_total-1, a single
  // contiguous block, so both sums collapse to two prefix lookups.
  auto rangeSum = [&](const std::vector<double>& prefix, int j) {
    const int lo = 0 - j - m_off;
    const int hi = (dict_total - 1) - j - m_off;
    return prefix[hi] - (lo > 0 ? prefix[lo - 1] : 0.0);
  };
  // Uniformly chosen occupied slot, for the death and shift proposals.
  auto pickPresent = [&](const std::vector<char>& present, size_t npresent) {
    size_t pick = fRNG.Integer(static_cast<UInt_t>(npresent));
    for (int k = 0; k < nslots; ++k) {
      if (!present[k]) continue;
      if (pick == 0) return k;
      --pick;
    }
    return -1;
  };
  auto sallOf = [&](int j) { return rangeSum(log1mp_prefix, j); };
  auto windowMass = [&](int j) { return rangeSum(phi_prefix, j); };
  auto pOcc = [&](int g, int j) { return std::min(0.99, std::max(1e-300, mu_eff * phi_tab[g - j - m_off] * dtp)); };
  // Full log prior log p(z|mu_eff,t0): the all-column baseline plus the log-odds
  // correction for the columns that are actually occupied.
  auto logPrior = [&](int j) {
    double s = sallOf(j);
    for (const Region& reg : regions) {
      for (int c : reg.active) {
        const double p = pOcc(reg.dict_start + c, j);
        s += std::log(p) - std::log1p(-p);
      }
    }
    return s;
  };

  // ---- sampler state, initialised from the seed configuration ----
  std::vector<char> present(nslots, 0);
  size_t npresent = 0;
  double w_present = 0.0;
  for (size_t r = 0; r < regions.size(); ++r) {
    for (int c : regions[r].active) {
      const int s = region_base[r] + c;
      if (present[s]) continue;
      present[s] = 1;
      ++npresent;
      w_present += slot_w[s];
    }
  }

  // t0 init: charge-weighted mean time of the initial PEs, else the grid centre.
  double wsum = 0.0, tsum = 0.0;
  for (const Region& reg : regions) {
    if (reg.charges.GetNrows() != static_cast<int>(reg.active.size())) continue;
    for (size_t k = 0; k < reg.active.size(); ++k) {
      const double w = std::max(0.0, reg.charges(static_cast<int>(k)));
      wsum += w;
      tsum += w * (reg.dict_start + reg.active[k]) * dtp;
    }
  }
  int jcur = (wsum > 0.0) ? static_cast<int>(std::lround(tsum / wsum / dtp)) : (g_min + g_max) / 2;
  jcur = std::min(jhi, std::max(jlo, jcur));

  double cur_logev = 0.0;
  for (const Region& reg : regions) cur_logev += reg.logev;
  double cur_logprior = logPrior(jcur);

  // MAP tracking (paper eq. 3.26, z_hat) and posterior accumulators.
  std::vector<std::vector<int>> best_active(regions.size());
  std::vector<TVectorD> best_charges(regions.size());
  for (size_t r = 0; r < regions.size(); ++r) {
    best_active[r] = regions[r].active;
    best_charges[r].ResizeTo(regions[r].charges);
    best_charges[r] = regions[r].charges;
  }
  double best_target = cur_logev + cur_logprior;

  double sum_t0 = 0.0, sum_mass = 0.0;
  size_t sum_npe = 0, n_rec = 0;
  std::vector<size_t> n_hist;

  // A shift jumps up to ~1 sample, so a PE can be repositioned in a few accepted
  // moves instead of dozens.
  const int shift_max = std::max(1, static_cast<int>(std::lround(upsample_factor)));
  const double t0_step_cols = std::max(t0_step / dtp, 1.0);

  // The common 1/3 move-type proposal factor cancels in every acceptance ratio
  // below, so it is omitted.
  const size_t total_iters = burn_in + n_mcmc_samples;

  for (size_t it = 0; it < total_iters; ++it) {
    // ---- z update: one birth/death/shift move over the whole waveform ----
    const double w_absent = total_w - w_present;
    const double move = fRNG.Uniform();

    if (move < 1.0 / 3.0 && npresent < static_cast<size_t>(nslots) && w_absent > 0.0) {
      // Birth: draw an unoccupied column with probability proportional to its
      // correlation weight.
      double u = fRNG.Uniform() * w_absent;
      int s = -1;
      for (int k = 0; k < nslots; ++k) {
        if (present[k]) continue;
        u -= slot_w[k];
        if (u <= 0.0) {
          s = k;
          break;
        }
      }
      if (s < 0) {  // rounding fallback: take the last unoccupied column
        for (int k = nslots - 1; k >= 0; --k) {
          if (!present[k]) {
            s = k;
            break;
          }
        }
      }
      if (s >= 0) {
        Region& reg = regions[slots[s].region];
        std::vector<int> trial = reg.active;
        trial.push_back(slots[s].col);
        std::sort(trial.begin(), trial.end());
        TVectorD ch;
        const double logev_p = LogEvidence(BuildActive(reg.W, reg.v.GetNrows(), trial), reg.v, ch);
        const double p = pOcc(slots[s].g, jcur);
        const double dlogprior = std::log(p) - std::log1p(-p);
        // q(z'->z) / q(z->z') for a weighted birth answered by a uniform death.
        const double logqratio = std::log(w_absent) - std::log(slot_w[s]) - std::log(static_cast<double>(npresent + 1));
        const double la = (logev_p - reg.logev) + dlogprior + logqratio;
        if (std::isfinite(logev_p) && std::log(fRNG.Uniform()) < la) {
          cur_logev += logev_p - reg.logev;
          reg.logev = logev_p;
          reg.active.swap(trial);
          reg.charges.ResizeTo(ch);
          reg.charges = ch;
          cur_logprior += dlogprior;
          present[s] = 1;
          ++npresent;
          w_present += slot_w[s];
        }
      }
    } else if (move < 2.0 / 3.0 && npresent > 0) {
      // Death: remove a uniformly chosen occupied column.
      const int s = pickPresent(present, npresent);
      if (s >= 0) {
        Region& reg = regions[slots[s].region];
        std::vector<int> trial = reg.active;
        trial.erase(std::find(trial.begin(), trial.end(), slots[s].col));
        TVectorD ch;
        const double logev_p = LogEvidence(BuildActive(reg.W, reg.v.GetNrows(), trial), reg.v, ch);
        const double p = pOcc(slots[s].g, jcur);
        const double dlogprior = std::log1p(-p) - std::log(p);
        // Reverse move is a weighted birth from the post-death absent weight.
        const double logqratio =
            std::log(slot_w[s]) - std::log(w_absent + slot_w[s]) + std::log(static_cast<double>(npresent));
        const double la = (logev_p - reg.logev) + dlogprior + logqratio;
        if (std::isfinite(logev_p) && std::log(fRNG.Uniform()) < la) {
          cur_logev += logev_p - reg.logev;
          reg.logev = logev_p;
          reg.active.swap(trial);
          reg.charges.ResizeTo(ch);
          reg.charges = ch;
          cur_logprior += dlogprior;
          present[s] = 0;
          --npresent;
          w_present -= slot_w[s];
        }
      }
    } else if (npresent > 0) {
      // Shift: move an occupied column up to shift_max columns within its own
      // region (symmetric proposal, so no Hastings ratio).
      const int s = pickPresent(present, npresent);
      if (s >= 0) {
        Region& reg = regions[slots[s].region];
        const int c = slots[s].col;
        const int dir = (fRNG.Uniform() < 0.5) ? -1 : 1;
        const int cp = c + dir * (1 + static_cast<int>(fRNG.Integer(static_cast<UInt_t>(shift_max))));
        const int sp = region_base[slots[s].region] + cp;
        if (cp >= 0 && cp < reg.dict_cols && !present[sp]) {
          std::vector<int> trial = reg.active;
          *std::find(trial.begin(), trial.end(), c) = cp;
          std::sort(trial.begin(), trial.end());
          TVectorD ch;
          const double logev_p = LogEvidence(BuildActive(reg.W, reg.v.GetNrows(), trial), reg.v, ch);
          const double p_old = pOcc(slots[s].g, jcur);
          const double p_new = pOcc(slots[sp].g, jcur);
          const double dlogprior = (std::log(p_new) - std::log1p(-p_new)) + (std::log1p(-p_old) - std::log(p_old));
          const double la = (logev_p - reg.logev) + dlogprior;
          if (std::isfinite(logev_p) && std::log(fRNG.Uniform()) < la) {
            cur_logev += logev_p - reg.logev;
            reg.logev = logev_p;
            reg.active.swap(trial);
            reg.charges.ResizeTo(ch);
            reg.charges = ch;
            cur_logprior += dlogprior;
            present[s] = 0;
            present[sp] = 1;
            w_present += slot_w[sp] - slot_w[s];
          }
        }
      }
    }

    // ---- t0 update: random-walk Metropolis on the grid index (the evidence
    // does not depend on t0, so only the prior enters) ----
    const int jstep = static_cast<int>(std::lround(fRNG.Gaus(0.0, t0_step_cols)));
    if (jstep != 0) {
      const int jprop = jcur + jstep;
      if (jprop >= jlo && jprop <= jhi) {
        const double lp_prop = logPrior(jprop);
        if (std::log(fRNG.Uniform()) < lp_prop - cur_logprior) {
          jcur = jprop;
          cur_logprior = lp_prop;
        }
      }
    }

    // ---- MAP tracking (every iteration) and recording (post burn-in) ----
    const double target = cur_logev + cur_logprior;
    if (target > best_target) {
      best_target = target;
      for (size_t r = 0; r < regions.size(); ++r) {
        best_active[r] = regions[r].active;
        best_charges[r].ResizeTo(regions[r].charges);
        best_charges[r] = regions[r].charges;
      }
    }
    if (it >= burn_in) {
      sum_t0 += jcur * dtp;
      sum_mass += windowMass(jcur);
      sum_npe += npresent;
      if (npresent >= n_hist.size()) n_hist.resize(npresent + 1, 0);
      ++n_hist[npresent];
      ++n_rec;
    }
  }

  for (size_t r = 0; r < regions.size(); ++r) {
    regions[r].active = best_active[r];
    regions[r].charges.ResizeTo(best_charges[r]);
    regions[r].charges = best_charges[r];
  }

  t0_hat = (n_rec > 0) ? sum_t0 / n_rec : jcur * dtp;
  npe_hat = (n_rec > 0) ? static_cast<double>(sum_npe) / n_rec : static_cast<double>(npresent);
  // IntensityMLE divides out the light-curve mass inside the recorded window.
  // It barely moves over the sampled t0 range, so the posterior mean suffices.
  const double mass_scale = (n_rec > 0) ? std::max(sum_mass / n_rec, 1e-3) : 1.0;
  mu_hat = (n_rec > 0) ? IntensityMLE(n_hist, mu_eff, mass_scale) : npe_hat;
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

void WaveformAnalysisFSMP::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
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
  if (regions.empty()) {
    // Nothing crossed threshold, so the model is flat. Still record it, so that
    // every analyzed PMT has an expected waveform to plot against.
    if (store_expected_waveform) fit_result->setExpectedWaveform(std::vector<double>(voltWfm.size(), 0.0));
    return;
  }

  // Starting configuration for the chain. A seeded PE count is the better
  // preconditioner intensity, since it comes from an algorithm that actually
  // resolves PEs rather than from the integrated charge.
  size_t nseed = 0;
  if (!seed_analyzer.empty()) nseed = SeedRegions(digitpmt, regions);
  if (nseed > 0) mu0 = static_cast<double>(nseed);

  double t0_hat = WaveformUtil::INVALID, mu_hat = 0.0, npe_hat = 0.0;
  SampleConfigurations(regions, fW.GetNcols(), mu0, t0_hat, mu_hat, npe_hat);

  // AddPE() cable-shifts the PE time but not the FOMs, so shift t0 here to keep
  // it on the same clock as the times it describes.
  const double t0_reported = (t0_hat == WaveformUtil::INVALID) ? t0_hat : t0_hat - digitpmt->GetTimeOffset();
  const std::map<std::string, double> extra_foms = {
      {"fsmp_t0", t0_reported}, {"fsmp_mu", mu_hat}, {"fsmp_npe", npe_hat}};

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
    EmitRegion(region, fit_result, gain_calibration, chi2ndf, extra_foms);
  }

  // The model behind the chi2ndf above: the MAP atoms convolved with the SER,
  // before EmitRegion's optional merging/NPE splitting reshapes them for output.
  if (store_expected_waveform) {
    fit_result->setExpectedWaveform(ExpectedWaveform(regions, fW, static_cast<int>(voltWfm.size())));
  }
}

}  // namespace RAT
