#include <TMath.h>

#include <RAT/DS/ChannelStatus.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/SERDictionary.hh>
#include <algorithm>
#include <cmath>

namespace RAT {

namespace {
// Per-channel width scales are bucketed before they reach the cache key: Eos
// has 272 channels spanning 0.98 to 1.63, and one dictionary per channel is
// ~1 MB each. 5% buckets keep the template within a twentieth of the SER width
// of the calibration while holding the cache to a dozen entries or so.
constexpr double kWidthScaleQuantum = 0.05;
// Above this many cached dictionaries, say so once -- it means the bucketing
// is not doing its job for this detector.
constexpr size_t kCacheWarnSize = 64;
}  // namespace

bool SERDictionary::TypeMap::Find(int pmt_type, double &out) const {
  for (size_t i = 0; i < types.size(); ++i) {
    if (types[i] == pmt_type) {
      out = values[i];
      return true;
    }
  }
  return false;
}

double SERDictionary::TypeMap::At(int pmt_type) const {
  double value = fallback;
  Find(pmt_type, value);
  return value;
}

SERDictionary::TypeMap SERDictionary::ReadTypeMap(DBLinkPtr tbl, const std::string &scalar,
                                                  const std::string &types_key, const std::string &values_key) const {
  TypeMap map;
  map.fallback = tbl->GetD(scalar);
  // The paired arrays are optional; without them every PMT gets the scalar.
  try {
    map.types = tbl->GetIArray(types_key);
    map.values = tbl->GetDArray(values_key);
  } catch (DBNotFoundError &) {
    map.types.clear();
    map.values.clear();
    return map;
  }
  if (map.types.size() != map.values.size()) {
    Log::Die(fOwner + ": " + types_key + "/" + values_key + " must have equal length.");
  }
  return map;
}

void SERDictionary::Configure(DBLinkPtr tbl, const std::string &owner) {
  fOwner = owner;

  fTemplateType = tbl->GetI("template_type");
  if (fTemplateType != kLognormal && fTemplateType != kGaussian && fTemplateType != kPMTPulse) {
    Log::Die(fOwner + ": Invalid template_type " + std::to_string(fTemplateType) +
             ". Must be 0 (lognormal), 1 (gaussian) or 2 (from PMTPULSE).");
  }
  // Read every family's parameters, not just the selected one, so that a macro
  // switching template_type with /rat/procset gets a configured template rather
  // than a zero-width one.
  try {
    fLognormalScale = tbl->GetD("lognormal_scale");
    fLognormalShape = tbl->GetD("lognormal_shape");
  } catch (DBNotFoundError &) {
    if (fTemplateType == kLognormal) throw;
    fLognormalScale = fLognormalShape = 0.0;
  }
  try {
    fGaussianWidth = ReadTypeMap(tbl, "gaussian_width", "gaussian_width_pmt_types", "gaussian_width_pmt_widths");
  } catch (DBNotFoundError &) {
    if (fTemplateType == kGaussian) throw;
    fGaussianWidth = TypeMap();
  }

  fVpeCharge = ReadTypeMap(tbl, "vpe_charge", "vpe_charge_pmt_types", "vpe_charge_pmt_charges");

  // The gamma prior is either the general (k, theta) pair, or -- per PMT type
  // -- a relative width, which fixes the prior mean at exactly one PE. The
  // latter is the natural form once vpe_charge is itself per type: k*theta = 1
  // and k*theta^2 = rel^2.
  fGammaK = tbl->GetD("gamma_k");
  fGammaTheta = tbl->GetD("gamma_theta");
  try {
    fGammaRelSigma.types = tbl->GetIArray("gamma_pmt_types");
    fGammaRelSigma.values = tbl->GetDArray("gamma_pmt_rel_sigma");
    if (fGammaRelSigma.types.size() != fGammaRelSigma.values.size()) {
      Log::Die(fOwner + ": gamma_pmt_types/gamma_pmt_rel_sigma must have equal length.");
    }
  } catch (DBNotFoundError &) {
    fGammaRelSigma.types.clear();
    fGammaRelSigma.values.clear();
  }

  try {
    fApplyWidthScale = tbl->GetZ("apply_pulse_width_scale");
  } catch (DBNotFoundError &) {
    fApplyWidthScale = false;
  }

  fCache.clear();
  fCacheWarned = false;
}

void SERDictionary::SetGeometry(int nsamples, double period, double term_ohms, double upsample_factor) {
  const double tol = 1e-9;
  if (nsamples == fNsamples && std::abs(period - fPeriod) < tol && std::abs(term_ohms - fTermOhms) < tol &&
      std::abs(upsample_factor - fUpsample) < tol) {
    return;
  }
  fNsamples = nsamples;
  fPeriod = period;
  fTermOhms = term_ohms;
  fUpsample = upsample_factor;
  fCache.clear();
}

int SERDictionary::ScaleBucket(double scale) const { return static_cast<int>(std::lround(scale / kWidthScaleQuantum)); }

double SERDictionary::WidthScale(int pmtid) const {
  if (!fApplyWidthScale) return 1.0;
  const double scale = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetPulseWidthScaleByPMTID(pmtid);
  if (!std::isfinite(scale) || scale <= 0.0) return 1.0;
  return ScaleBucket(scale) * kWidthScaleQuantum;
}

double SERDictionary::VpeCharge(int pmtid) const {
  return fVpeCharge.At(DS::RunStore::GetCurrentRun()->GetPMTInfo()->GetType(pmtid));
}

// A listed PMT type gets a prior of mean exactly one PE and relative width
// `rel`, i.e. k = 1/rel^2 and theta = rel^2. An unlisted one keeps the general
// (k, theta) pair, whose mean need not be 1 -- so these cannot share a
// fallback, and the lookup has to report whether it found the type.
double SERDictionary::GammaK(int pmtid) const {
  double rel = 0.0;
  if (!fGammaRelSigma.Find(DS::RunStore::GetCurrentRun()->GetPMTInfo()->GetType(pmtid), rel) || rel <= 0.0) {
    return fGammaK;
  }
  return 1.0 / (rel * rel);
}

double SERDictionary::GammaTheta(int pmtid) const {
  double rel = 0.0;
  if (!fGammaRelSigma.Find(DS::RunStore::GetCurrentRun()->GetPMTInfo()->GetType(pmtid), rel) || rel <= 0.0) {
    return fGammaTheta;
  }
  return rel * rel;
}

SERDictionary::Key SERDictionary::KeyFor(int pmtid) const {
  DS::PMTInfo *pmtinfo = DS::RunStore::GetCurrentRun()->GetPMTInfo();
  Key key;
  key.kind = fTemplateType;
  key.scale_bucket = ScaleBucket(WidthScale(pmtid));
  key.vpe_fc = static_cast<int>(std::lround(VpeCharge(pmtid) * 1000.0));
  if (fTemplateType == kGaussian) {
    key.width_ps = static_cast<int>(std::lround(fGaussianWidth.At(pmtinfo->GetType(pmtid)) * 1000.0));
  } else if (fTemplateType == kPMTPulse) {
    key.model = pmtinfo->GetModel(pmtid);
  }
  return key;
}

double SERDictionary::Shape::Evaluate(double dt) const {
  switch (kind) {
    case kAnalyticLognormal:
      // RAT::PMTPulse's convention: defined for dt > -m, peaking just before 0.
      if (dt <= -lognormal_m) return 0.0;
      return TMath::LogNormal(dt, lognormal_s, -lognormal_m, lognormal_m);
    case kAnalyticGaussian:
      return TMath::Gaus(dt, 0.0, sigma, kTRUE);
    case kGaussianMixture: {
      // The simulation draws a fresh width per PE, so the SER of an ensemble of
      // PEs is the width distribution's mixture, not any one gaussian.
      double val = 0.0;
      for (size_t i = 0; i < widths.size(); ++i) {
        val += width_probs[i] * TMath::Gaus(dt, 0.0, widths[i], kTRUE);
      }
      return val;
    }
    case kTabulated: {
      if (dt < times.front() || dt > times.back()) return 0.0;
      const size_t i = std::upper_bound(times.begin(), times.end(), dt) - times.begin();
      if (i == 0) return values.front();
      if (i >= times.size()) return values.back();
      const double span = times[i] - times[i - 1];
      if (span <= 0.0) return values[i - 1];
      return values[i - 1] + (values[i] - values[i - 1]) * (dt - times[i - 1]) / span;
    }
  }
  return 0.0;
}

SERDictionary::Shape SERDictionary::ShapeFromPMTPulse(const std::string &model_name) const {
  DBLinkPtr lpulse;
  try {
    lpulse = DB::Get()->GetLink("PMTPULSE", model_name);
    lpulse->GetS("index");
  } catch (DBNotFoundError &) {
    try {
      lpulse = DB::Get()->GetLink("PMTPULSE", "");
      lpulse->GetS("index");
      warn << fOwner << ": no PMTPULSE table for model \"" << model_name << "\", using the default one." << newline;
    } catch (DBNotFoundError &) {
      Log::Die(fOwner + ": template_type 2 needs a PMTPULSE table for model \"" + model_name +
               "\", and there is no default either.");
    }
  }

  std::string pulse_type = "analytic";
  try {
    pulse_type = lpulse->GetS("pulse_type");
  } catch (DBNotFoundError &) {
  }

  Shape shape;
  if (pulse_type == "datadriven") {
    shape.kind = Shape::kTabulated;
    shape.times = lpulse->GetDArray("pulse_shape_times");
    shape.values = lpulse->GetDArray("pulse_shape_values");
    if (shape.times.size() != shape.values.size() || shape.times.size() < 2) {
      Log::Die(fOwner + ": PMTPULSE[" + model_name + "] pulse_shape_times/values must pair up, 2 points minimum.");
    }
    // A column of weight 1 has to carry exactly vpe_charge, so the tabulated
    // shape is renormalised to unit integral the way PMTWaveformGenerator does.
    double integral = 0.0;
    for (size_t i = 0; i + 1 < shape.times.size(); ++i) {
      integral += (shape.times[i + 1] - shape.times[i]) * (shape.values[i] + shape.values[i + 1]) / 2.0;
    }
    if (integral <= 0.0) Log::Die(fOwner + ": PMTPULSE[" + model_name + "] pulse shape has non-positive integral.");
    for (double &v : shape.values) v /= integral;
    return shape;
  }

  std::string pulse_shape = "lognormal";
  try {
    pulse_shape = lpulse->GetS("pulse_shape");
  } catch (DBNotFoundError &) {
  }

  if (pulse_shape == "gaussian") {
    const std::vector<double> widths = lpulse->GetDArray("gaussian_width");
    const std::vector<double> probs = lpulse->GetDArray("gaussian_width_prob");
    if (widths.size() != probs.size() || widths.size() < 2) {
      Log::Die(fOwner + ": PMTPULSE[" + model_name + "] gaussian_width/gaussian_width_prob must pair up.");
    }
    shape.kind = Shape::kGaussianMixture;
    shape.widths = widths;
    // Trapezoid weights over a possibly non-uniform width grid, normalised to
    // sum to 1 so the mixture keeps unit integral in time.
    shape.width_probs.assign(widths.size(), 0.0);
    double total = 0.0;
    for (size_t i = 0; i < widths.size(); ++i) {
      const double lo = (i == 0) ? widths[0] : widths[i - 1];
      const double hi = (i + 1 == widths.size()) ? widths.back() : widths[i + 1];
      const double w = probs[i] * (hi - lo) / 2.0;
      shape.width_probs[i] = w;
      total += w;
    }
    if (total <= 0.0) Log::Die(fOwner + ": PMTPULSE[" + model_name + "] width distribution has no weight.");
    for (double &w : shape.width_probs) w /= total;
    return shape;
  }

  shape.kind = Shape::kAnalyticLognormal;
  shape.lognormal_m = lpulse->GetD("lognormal_mean");
  shape.lognormal_s = lpulse->GetD("lognormal_width");
  return shape;
}

SERDictionary::Shape SERDictionary::ShapeFor(const Key &key, int pmtid) const {
  const double scale = WidthScale(pmtid);
  Shape shape;
  if (key.kind == kLognormal) {
    shape.kind = Shape::kAnalyticLognormal;
    shape.lognormal_m = fLognormalScale;
    shape.lognormal_s = fLognormalShape;
  } else if (key.kind == kGaussian) {
    shape.kind = Shape::kAnalyticGaussian;
    shape.sigma = key.width_ps / 1000.0;
  } else {
    shape = ShapeFromPMTPulse(DS::RunStore::GetCurrentRun()->GetPMTInfo()->GetModelName(key.model));
  }

  if (scale != 1.0) {
    // Stretch the template exactly as PMTWaveformGenerator stretches the pulse
    // it generated: widths and the lognormal mean scale, and a tabulated shape
    // scales in time and is renormalised.
    switch (shape.kind) {
      case Shape::kAnalyticLognormal:
        shape.lognormal_m *= scale;
        break;
      case Shape::kAnalyticGaussian:
        shape.sigma *= scale;
        break;
      case Shape::kGaussianMixture:
        for (double &w : shape.widths) w *= scale;
        break;
      case Shape::kTabulated:
        for (double &t : shape.times) t *= scale;
        for (double &v : shape.values) v /= scale;
        break;
    }
  }
  return shape;
}

void SERDictionary::Build(const Key &key, int pmtid, TMatrixD &out) const {
  const Shape shape = ShapeFor(key, pmtid);

  const int dict_size = static_cast<int>(fNsamples * fUpsample);
  out.ResizeTo(fNsamples, dict_size);
  out.Zero();

  // mag_factor maps the unit-integral time PDF [1/ns] to a voltage [mV] such
  // that a column with weight 1 corresponds to a PE of vpe_charge:
  // charge[pC] = -V[mV] * dt[ns] / Ohm  =>  integral(template)*dt = vpe_charge.
  const double mag_factor = (key.vpe_fc / 1000.0) * fTermOhms;

  for (int col = 0; col < dict_size; ++col) {
    const double delay = col * fPeriod / fUpsample;
    for (int row = 0; row < fNsamples; ++row) {
      // Pulses are negative-going, matching the pedestal-subtracted waveform.
      out(row, col) = -mag_factor * shape.Evaluate(row * fPeriod - delay);
    }
  }
}

const TMatrixD &SERDictionary::Get(int pmtid) {
  if (fNsamples <= 0 || fUpsample <= 0.0) {
    Log::Die(fOwner + ": SERDictionary used before SetGeometry().");
  }
  const Key key = KeyFor(pmtid);
  auto it = fCache.find(key);
  if (it == fCache.end()) {
    it = fCache.emplace(key, TMatrixD()).first;
    debug << fOwner << ": building dictionary " << fNsamples << " x " << static_cast<int>(fNsamples * fUpsample)
          << " for template kind " << key.kind << ", model " << key.model << ", width " << key.width_ps << " ps"
          << newline;
    Build(key, pmtid, it->second);
    if (fCache.size() > kCacheWarnSize && !fCacheWarned) {
      warn << fOwner << ": " << fCache.size()
           << " distinct SER dictionaries cached. Check apply_pulse_width_scale bucketing." << newline;
      fCacheWarned = true;
    }
  }
  return it->second;
}

bool SERDictionary::SetD(const std::string &param, double value) {
  if (param == "lognormal_scale") {
    fLognormalScale = value;
  } else if (param == "lognormal_shape") {
    fLognormalShape = value;
  } else if (param == "gaussian_width") {
    fGaussianWidth.fallback = value;
  } else if (param == "vpe_charge") {
    fVpeCharge.fallback = value;
  } else if (param == "gamma_k") {
    fGammaK = value;
  } else if (param == "gamma_theta") {
    fGammaTheta = value;
  } else {
    return false;
  }
  fCache.clear();
  return true;
}

bool SERDictionary::SetI(const std::string &param, int value) {
  if (param == "template_type") {
    if (value != kLognormal && value != kGaussian && value != kPMTPulse) {
      Log::Die(fOwner + ": Invalid template_type " + std::to_string(value) +
               ". Must be 0 (lognormal), 1 (gaussian) or 2 (from PMTPULSE).");
    }
    fTemplateType = value;
  } else if (param == "apply_pulse_width_scale") {
    fApplyWidthScale = (value != 0);
  } else {
    return false;
  }
  fCache.clear();
  return true;
}

}  // namespace RAT
