#include <RAT/G4CerenkovProcess.hh>
#include <RAT/Log.hh>
#include <RAT/MuteGeant4.hh>
#include <RAT/ThinnableG4Cerenkov.hh>
#include <vector>

namespace RAT {

ThinnableG4Cerenkov::ThinnableG4Cerenkov()
    : should_thin(false), thinning_factor(1.0), lower_wavelength_threshold(0.0), upper_wavelength_threshold(2000.0) {
  this->heprandom = CLHEP::HepRandom();
}

void ThinnableG4Cerenkov::SetThinningFactor(double thinning) {
  if ((thinning > 0) && (thinning < 1.0)) {
    this->should_thin = true;
    this->thinning_factor = thinning;
  } else if (thinning == 1.0) {
    this->should_thin = this->should_thin || false;
    this->thinning_factor = thinning;
  } else {
    Log::Die(dformat("Cannot thin photons with acceptance %1.f%", thinning));
  }
}

double ThinnableG4Cerenkov::GetThinningFactor() {
  double rv = this->thinning_factor;
  return rv;
}

void ThinnableG4Cerenkov::SetLowerWavelengthThreshold(double wvl_thresh_lo) {
  if (wvl_thresh_lo > 0) {
    this->should_thin = true;
    this->lower_wavelength_threshold = wvl_thresh_lo;
  } else if (wvl_thresh_lo == 0) {
    this->should_thin = this->should_thin || false;
    this->lower_wavelength_threshold = wvl_thresh_lo;
  } else {
    Log::Die(dformat("Cannot set lower wavelength threshold for Cerenkov photons below 0: %1.f%", wvl_thresh_lo));
  }
}

double ThinnableG4Cerenkov::GetLowerWavelengthThreshold() {
  double rv = this->lower_wavelength_threshold;
  return rv;
}

void ThinnableG4Cerenkov::SetUpperWavelengthThreshold(double wvl_thresh_hi) {
  if (wvl_thresh_hi < 2000) {  // Ignore any threshold sufficiently above visible light
    this->should_thin = true;
    this->upper_wavelength_threshold = wvl_thresh_hi;
  }
}

double ThinnableG4Cerenkov::GetUpperWavelengthThreshold() {
  double rv = this->upper_wavelength_threshold;
  return rv;
}

G4VParticleChange *ThinnableG4Cerenkov::PostStepDoIt(const G4Track &aTrack, const G4Step &aStep) {
  // Let G4 predict how many photons should be produced.
  G4VParticleChange *rv = G4CerenkovProcess::PostStepDoIt(aTrack, aStep);

  if (!should_thin) {
    return rv;
  }

  // Only propagate photons that pass the thinning criteria.
  G4int n_pred = rv->GetNumberOfSecondaries();
  if (n_pred == 0) {
    return rv;
  }

  // Reset the secondary count and reuse the existing storage.
  rv->SetNumberOfSecondaries(n_pred);

  for (G4int i = 0; i < n_pred; i++) {
    G4Track *existing = rv->GetSecondary(i);

    // Calculate photon wavelength.
    double wvl = (CLHEP::twopi * CLHEP::hbarc) / (existing->GetKineticEnergy() * CLHEP::MeV) / CLHEP::nm;

    // Check whether the photon wavelength is within the allowed range.
    bool in_range = wvl >= lower_wavelength_threshold && wvl <= upper_wavelength_threshold;

    if (in_range && heprandom.flat() < this->GetThinningFactor()) {
      existing->SetParentID(aTrack.GetTrackID());
      rv->AddSecondary(existing);
    } else {
      delete existing;
    }
  }

  return rv;
}

}  // namespace RAT
