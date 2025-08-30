#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>
#include <fftw3.h>

#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/ROOTInterpolator.hh>
#include <RAT/WaveformAnalysisLucyDDM.hh>
#include <chrono>
#include <complex>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {
void WaveformAnalysisLucyDDM::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    vpe_scale = fDigit->GetD("vpe_scale");
    vpe_shape = fDigit->GetD("vpe_shape");
    vpe_charge = fDigit->GetD("vpe_charge");
    ds = fDigit->GetD("internal_sampling_period");
    vpe_nsamples = fDigit->GetI("vpe_nsamples");
    roi_threshold = fDigit->GetD("roi_threshold");
    max_iterations = fDigit->GetI("max_iterations");
    stopping_nll_diff = fDigit->GetD("stopping_nll_diff");
    peak_height_threshold = fDigit->GetD("peak_height_threshold");
    charge_threshold = fDigit->GetD("charge_threshold");
    min_peak_distance = fDigit->GetD("min_peak_distance");
    npe_estimate = fDigit->GetZ("npe_estimate");
    npe_estimate_charge_width = fDigit->GetD("npe_estimate_charge_width");
    npe_estimate_max_pes = fDigit->GetI("npe_estimate_max_pes");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisLucyDDM: Unable to find analysis parameters.");
  }
  vpe_norm.resize(vpe_nsamples);
  vpe_norm_flipped.resize(vpe_nsamples);
  for (size_t idx = 0; idx < vpe_nsamples; ++idx) {
    double curr_time = idx * ds;
    vpe_norm.at(idx) = TMath::LogNormal(curr_time, vpe_shape, 0, vpe_scale);
    vpe_norm_flipped.at(vpe_nsamples - 1 - idx) = vpe_norm.at(idx);
  }
}

void WaveformAnalysisLucyDDM::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double gain_calibration = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetChargeScaleByPMTID(digitpmt->GetID());
  vpe_integral = vpe_charge * fTermOhms * gain_calibration;
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, -fVoltageRes, digitpmt->GetPedestal());
  ClampBelowThreshold(voltWfm);
  // We perform "full" size convolution throughout this calculation. The upsampled waveform is convolved with the kernel
  // twice.
  size_t fft_size = std::ceil((voltWfm.size() - 1) * fTimeStep / ds)  // upsampled waveform
                    + (vpe_nsamples - 1) + (vpe_nsamples - 1);        // two convolutions with the kernel
  RequestFFTSize(fft_size);
  auto start = std::chrono::high_resolution_clock::now();
  size_t iterations_ran = 0;
  std::vector<double> demod_result = Deconvolve(voltWfm, iterations_ran);
  auto end = std::chrono::high_resolution_clock::now();
  debug << "Deconvolution took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms."
        << newline;
  std::vector<double> reblurred_wfm = ReblurWaveform(demod_result);
  double poisson_nll = PoissonNLL(Resample(voltWfm, reblurred_wfm.size()), reblurred_wfm);
  std::vector<double> reco_times, reco_charges, reco_time_errors, reco_charge_errors;
  double chi2ndf;
  start = std::chrono::high_resolution_clock::now();
  FindHits(demod_result, reco_times, reco_charges, reco_time_errors, reco_charge_errors, chi2ndf);
  if (min_peak_distance > 0) {
    MergeClosePeaks(reco_times, reco_charges, reco_time_errors, reco_charge_errors);
  }
  end = std::chrono::high_resolution_clock::now();
  debug << "Hit finding took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms."
        << newline;
  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("LucyDDM");
  for (size_t ipacket = 0; ipacket < reco_times.size(); ++ipacket) {
    size_t npe = EstimateNPE(reco_charges[ipacket]);
    for (size_t i = 0; i < npe; ++i) {
      fit_result->AddPE(reco_times[ipacket], reco_charges[ipacket] / npe,
                        {
                            {"time_error", reco_time_errors[ipacket]},
                            {"charge_error", reco_charge_errors[ipacket] / npe},
                            {"chi2ndf", chi2ndf},
                            {"poisson_nll", poisson_nll},
                            {"iterations_ran", iterations_ran},
                            {"estimated_npe", npe},
                        });
    }
  }
}

void WaveformAnalysisLucyDDM::MergeClosePeaks(std::vector<double>& times, std::vector<double>& charges,
                                              std::vector<double>& time_errors, std::vector<double>& charge_errors) {
  if (times.size() <= 1) {
    return;
  }
  std::vector<double> merged_times;
  merged_times.reserve(times.size());
  std::vector<double> merged_charges;
  merged_charges.reserve(charges.size());
  std::vector<double> merged_time_errors;
  merged_time_errors.reserve(time_errors.size());
  std::vector<double> merged_charge_errors;
  merged_charge_errors.reserve(charge_errors.size());

  auto add_merged_peak = [&](size_t beg, size_t end) -> void {
    size_t num = end - beg + 1;
    double mean_time = std::accumulate(times.begin() + beg, times.begin() + end + 1, 0.0) / num;
    // NOTE: This is rather arbitrary, and might not be the best metric for the error. But this is typically much bigger
    // than the MINUIT error associated with each peak.
    double time_error =
        (num > 1) ? TMath::StdDev(times.begin() + beg, times.begin() + end + 1) / std::sqrt(num) : time_errors[beg];
    double total_charge = std::accumulate(charges.begin() + beg, charges.begin() + end + 1, 0.0);
    double charge_error = std::sqrt(std::accumulate(charge_errors.begin() + beg, charge_errors.begin() + end + 1, 0.0,
                                                    [](double a, double b) { return a + b * b; }));
    merged_times.push_back(mean_time);
    merged_charges.push_back(total_charge);
    merged_time_errors.push_back(time_error);
    merged_charge_errors.push_back(charge_error);
  };

  size_t curr_beg = 0;
  size_t curr_end = 0;
  for (size_t idx = 1; idx < times.size(); idx++) {
    if ((times[idx] - times[curr_end]) <= min_peak_distance) {
      // grow current cluster
      curr_end = idx;
    } else {
      add_merged_peak(curr_beg, curr_end);
      curr_beg = idx;
      curr_end = idx;
    }
  }
  add_merged_peak(curr_beg, curr_end);
  times = std::move(merged_times);
  charges = std::move(merged_charges);
  time_errors = std::move(merged_time_errors);
  charge_errors = std::move(merged_charge_errors);
}

static constexpr size_t next_power_of_two(size_t n) {
  size_t power = 1;
  while (power < n) power <<= 1;
  return power;
}

void WaveformAnalysisLucyDDM::RequestFFTSize(size_t size) {
  if (!fft || fft->GetSize() < size) {
    size_t fft_size = next_power_of_two(size);
    info << "WaveformAnalysisLucyDDM: Creating new FFT transformers of size " << fft_size << newline;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    fft = std::make_unique<FFTW1DTransformer>(fft_size, FFTW1DTransformer::direction_t::FORWARD,
                                              FFTW_PATIENT | FFTW_DESTROY_INPUT);
    ifft = std::make_unique<FFTW1DTransformer>(fft_size, FFTW1DTransformer::direction_t::INVERSE,
                                               FFTW_PATIENT | FFTW_DESTROY_INPUT);
    // compute FFT for the kernels
    vpe_norm_fft = fft->transform(vpe_norm);
    vpe_norm_flipped_fft = fft->transform(vpe_norm_flipped);
    info << "Plan Creation took "
         << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
                .count()
         << " ms." << newline;
  }
}

std::vector<double> WaveformAnalysisLucyDDM::ConvolveFFT(const std::vector<double>& a,
                                                         const std::vector<std::complex<double>>& b_fft,
                                                         size_t conv_size, double dt) const {
  size_t complex_fft_size = fft->GetSize() / 2 + 1;
  std::vector<std::complex<double>> a_fft = fft->transform(a);
  std::complex<double>* product = reinterpret_cast<std::complex<double>*>(fftw_alloc_complex(complex_fft_size));
  for (size_t iw = 0; iw < complex_fft_size; ++iw) {
    double re = a_fft[iw].real() * b_fft[iw].real() - a_fft[iw].imag() * b_fft[iw].imag();
    double im = a_fft[iw].real() * b_fft[iw].imag() + a_fft[iw].imag() * b_fft[iw].real();
    product[iw] = std::complex<double>(re, im);
  }
  std::vector<double> result(ifft->GetSize());
  ifft->ifft(product, result.data());
  fftw_free(product);
  result.resize(conv_size);
  for (double& val : result) val *= dt;
  return result;
}

std::vector<double> WaveformAnalysisLucyDDM::Deconvolve(const std::vector<double>& wfm, size_t& iterations_ran) const {
  double t_max = (wfm.size() - 1) * fTimeStep;

  // define the timing axis for the different full convolutions.
  // Waveform spans [0, tmax]. Template spans [0, t_vpe]
  // `s` is the time axis for phi, which should span [0, tmax]
  // `t` is the time axis for conv(phi, v_pe), which spans [0, tmax + t_vpe]

  size_t s_axis_nsamples = static_cast<size_t>(std::ceil(t_max / ds));
  size_t t_axis_nsamples = s_axis_nsamples + vpe_nsamples - 1;

  // Resample waveform in prep of dividing by conv(phi, v_pe)
  std::vector<double> wfm_resampled = Resample(wfm, t_axis_nsamples);
  ClampBelowThreshold(wfm_resampled, epsilon);
  size_t current_iteration = 0;
  std::vector<double> current_phi(s_axis_nsamples, 1);
  double prev_nll = 1e9;
  while (current_iteration <= max_iterations) {
    std::vector<double> reblurred_wfm = ReblurWaveform(current_phi);
    if (reblurred_wfm.size() != t_axis_nsamples) {
      Log::Die("WaveformAnalysisLucyDDM: Convolution result size does not match expected size.");
    }

    double curr_nll = PoissonNLL(wfm_resampled, reblurred_wfm);
    if ((curr_nll < prev_nll) && (prev_nll - curr_nll) < stopping_nll_diff) {
      break;
    }
    prev_nll = curr_nll;
    std::vector<double> frac(t_axis_nsamples);
    for (size_t it = 0; it < t_axis_nsamples; ++it) {
      frac[it] = wfm_resampled[it] / reblurred_wfm[it];
    }
    // vpe_norm_flipped has time [-t_vpe, 0]. Frac has [0, tmax + t_vpe]
    // The output has [ -t_vpe, tmax + t_vpe]
    std::vector<double> factor = ConvolveFFT(frac, vpe_norm_flipped_fft, frac.size() + vpe_nsamples - 1, ds);
    // but this gets multiplied to phi, which just has [0, tmax]
    std::vector<double> next_phi(s_axis_nsamples, 0);
    for (size_t is = 0; is < s_axis_nsamples; ++is) {
      // phi is defined on [0, tmax], so we need to shift the factor by vpe_nsamples - 1
      next_phi[is] = current_phi[is] * factor[is + vpe_nsamples - 1];
    }
    current_phi = std::move(next_phi);
    current_iteration++;
  }
  iterations_ran = current_iteration;
  return current_phi;
}

void WaveformAnalysisLucyDDM::FindHits(const std::vector<double>& phi, std::vector<double>& out_times,
                                       std::vector<double>& out_charges, std::vector<double>& out_time_errors,
                                       std::vector<double>& out_charge_errors, double& chi2ndf) const {
  // Find hits in the deconvolved waveform
  std::vector<size_t> peak_idx = WaveformUtil::FindPeaks(phi, peak_height_threshold);
  if (peak_idx.empty()) {
    warn << "WaveformAnalysisLucyDDM: No peaks found in the deconvolved waveform. Inserting max sample to for fit "
            "attempt anyways."
         << newline;
    size_t max_idx = std::distance(phi.begin(), std::max_element(phi.begin(), phi.end()));
    peak_idx.push_back(max_idx);
  }
  std::vector<double> peak_times, peak_heights;
  size_t n_peaks = peak_idx.size();
  for (size_t idx : peak_idx) {
    peak_times.push_back(idx * ds);
    peak_heights.push_back(phi[idx]);
  }
  std::vector<double> s_axis(phi.size());
  for (size_t is = 0; is < phi.size(); ++is) {
    s_axis[is] = is * ds;
  }
  TGraph phi_graph(s_axis.size(), s_axis.data(), phi.data());
  TF1* pulse_train = new TF1(
      "pulse_train", [this, n_peaks](double* x, double* p) { return this->GaussianPulseTrain(x, p, n_peaks); }, 0,
      ds * phi.size(), 3 * n_peaks);
  for (size_t ipulse = 0; ipulse < n_peaks; ++ipulse) {
    pulse_train->SetParameter(3 * ipulse, peak_heights[ipulse]);
    pulse_train->SetParLimits(3 * ipulse, 0, peak_heights[ipulse] * 2);
    pulse_train->SetParameter(3 * ipulse + 1, peak_times[ipulse]);
    pulse_train->SetParLimits(3 * ipulse + 1, std::max(-vpe_scale, peak_times[ipulse] - fTimeStep * 2),
                              std::min(peak_times[ipulse] + fTimeStep * 2, ds * phi.size() - vpe_scale));
    pulse_train->SetParameter(3 * ipulse + 2, ds);
    pulse_train->SetParLimits(3 * ipulse + 2, ds / 10, ds * 10);
  }
  phi_graph.Fit(pulse_train, "0QR");
  for (size_t ipulse = 0; ipulse < n_peaks; ++ipulse) {
    double t0 = pulse_train->GetParameter(3 * ipulse + 1);
    double t0_error = pulse_train->GetParError(3 * ipulse + 1);
    double A = pulse_train->GetParameter(3 * ipulse);
    double A_error = pulse_train->GetParError(3 * ipulse);
    double sigma = pulse_train->GetParameter(3 * ipulse + 2);
    double sigma_error = pulse_train->GetParError(3 * ipulse + 2);
    double charge = A * sigma * std::sqrt(2 * TMath::Pi()) * vpe_charge;
    if (charge < charge_threshold) continue;
    double charge_error =
        charge * std::sqrt((A_error / A) * (A_error / A) + (sigma_error / sigma) * (sigma_error / sigma));
    out_times.push_back(t0 + vpe_scale);
    out_charges.push_back(charge);
    out_time_errors.push_back(t0_error);
    out_charge_errors.push_back(charge_error);
  }
  std::vector<size_t> out_idx(out_times.size());
  TMath::Sort(out_times.size(), out_times.data(), out_idx.data(), false);
  auto reorder = [&](std::vector<double>& vec) {
    std::vector<double> tmp(vec.size());
    for (size_t i = 0; i < vec.size(); i++) tmp[i] = vec[out_idx[i]];
    vec = std::move(tmp);
  };
  reorder(out_times);
  reorder(out_charges);
  reorder(out_time_errors);
  reorder(out_charge_errors);
  chi2ndf = pulse_train->GetChisquare() / pulse_train->GetNDF();
}

size_t WaveformAnalysisLucyDDM::EstimateNPE(double charge) const {
  if (!npe_estimate) {
    return 1;
  }
  std::vector<double> log_likelihood(npe_estimate_max_pes, 0.0);
  for (size_t npe = 1; npe <= npe_estimate_max_pes; ++npe) {
    log_likelihood[npe - 1] =
        -std::pow(charge - npe * vpe_charge, 2) / (2 * npe * std::pow(npe_estimate_charge_width, 2)) -
        0.5 * std::log(2 * TMath::Pi() * npe * std::pow(npe_estimate_charge_width, 2));
  }
  return std::distance(log_likelihood.begin(), std::max_element(log_likelihood.begin(), log_likelihood.end())) + 1;
}

void WaveformAnalysisLucyDDM::ClampBelowThreshold(std::vector<double>& wfm, double thresh) const {
  if (thresh == -9999) {
    thresh = roi_threshold;
  }
  for (double& sample : wfm) {
    if (sample < thresh) {
      sample = epsilon;
    }
  }
}

double WaveformAnalysisLucyDDM::PoissonNLL(const std::vector<double>& wfm,
                                           const std::vector<double>& reblurred_wfm) const {
  double result = 0;
  if (wfm.size() != reblurred_wfm.size()) {
    Log::Die("WaveformAnalysisLucyDDM: Waveform and reblurred waveform sizes do not match.");
  }
  for (size_t it = 0; it < wfm.size(); ++it) {
    result -= wfm[it] * std::log(reblurred_wfm[it]) - reblurred_wfm[it];
  }
  return result;
}

std::vector<double> WaveformAnalysisLucyDDM::ReblurWaveform(const std::vector<double>& phi) const {
  // Reblur the waveform using the vpe template
  std::vector<double> result = ConvolveFFT(phi, vpe_norm_fft, phi.size() + vpe_nsamples - 1, ds);
  for (double& val : result) val *= vpe_integral;
  ClampBelowThreshold(result, epsilon);
  return result;
}

std::vector<double> WaveformAnalysisLucyDDM::Resample(const std::vector<double>& wfm, size_t n_samples) const {
  std::vector<double> result(n_samples, 0.0);
  std::vector<double> wfm_times(wfm.size());
  for (size_t it = 0; it < wfm.size(); ++it) {
    wfm_times[it] = it * fTimeStep;
  }
  ROOTInterpolator wfm_interpolator(wfm_times, wfm, ROOTInterpolator::kind_t::kLinear,
                                    ROOTInterpolator::extrapolation_t::kClamp);
  for (size_t idx = 0; idx < n_samples; ++idx) {
    double tt = idx * ds;
    result[idx] = wfm_interpolator(tt);
  }
  return result;
}

// Fourier interpolation. Doesn't seem to work well...
// std::vector<double> WaveformAnalysisLucyDDM::Resample(const std::vector<double>& wfm, size_t n_samples) const {
//   FFTW1DTransformer resampler_fft(wfm.size(), FFTW1DTransformer::direction_t::FORWARD,
//                                                   FFTW_ESTIMATE);
//   size_t s_axis_nsamples = static_cast<size_t>(std::ceil(wfm.size() * fTimeStep / ds));
//   FFTW1DTransformer resampler_ifft(s_axis_nsamples, FFTW1DTransformer::direction_t::INVERSE,
//                                                   FFTW_ESTIMATE);
//   std::vector<std::complex<double>>wfm_fft = resampler_fft.transform(wfm);
//   wfm_fft.resize(s_axis_nsamples / 2 + 1, 0.0);
//   std::vector<double> result = resampler_ifft.transform(wfm_fft);
//   result.resize(n_samples, 0.0);
//   return result;
// }

double WaveformAnalysisLucyDDM::GaussianPulseTrain(double* x, double* p, size_t N) const {
  double val = 0.0;
  // parameter list is [amplitude, mean, sigma] for each pulse.
  for (size_t ipulse = 0; ipulse < N; ++ipulse) {
    double A = p[3 * ipulse];
    double t0 = p[3 * ipulse + 1];
    double sigma = p[3 * ipulse + 2];
    double dx = (x[0] - t0) / sigma;
    val += A * std::exp(-0.5 * dx * dx);
  }
  return val;
}
}  // namespace RAT
