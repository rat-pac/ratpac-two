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
  // let G4 predict how many photons should be produced...
  G4VParticleChange *rv = G4Cerenkov::PostStepDoIt(aTrack, aStep);

  if (!should_thin) {
    return rv;
  }

  // but only choose a fraction thereof to actually propagate
  G4int n_pred = rv->GetNumberOfSecondaries();
  if (n_pred == 0) {
    return rv;
  }
  // TODO a fixed length here could speed things up...
  // but is not (?) predictable, since we don't know outcome of following
  // per-photon RNG decisions
  //  G4int n_prod = static_cast<G4int>(std::round(n_pred));
  std::vector<G4Track *> secondaries;
  for (G4int i = 0; i < n_pred; i++) {
    G4Track *existing = rv->GetSecondary(i);

    // Check whether wavelength in range and continue if not
    double wvl = (CLHEP::twopi * CLHEP::hbarc) / (existing->GetKineticEnergy() * CLHEP::MeV) / CLHEP::nm;
    if (wvl < lower_wavelength_threshold || wvl > upper_wavelength_threshold) {
      continue;
    }

    double random = heprandom.flat();
    if (random < this->GetThinningFactor()) {
      // G4VParticleChange::SetNumberOfSecondaries will free all pending
      // secondaries, so we must explicitly copy each surviving track
      // into a separate data structure, which is a two-step process
      // first, copy data structure
      G4Track *secondary = new G4Track(*existing);
      // second, explicitly label tracking
      secondary->SetParentID(aTrack.GetTrackID());
      secondaries.push_back(secondary);
    }
  }

  // clear the list of secondaries, and repopulate with chosen subset
  // Geant4 may spit out warning messages based on whether it was compiled with
  // verbose flags but this is safe to ignore, though it will clutter the log
  // UPDATE: Going to just redirect the output to ignore this since we are intentionally
  // calling this function again and the prints at issue got removed in G4 11.1.0
  RAT::mute_g4mute();
  rv->SetNumberOfSecondaries(secondaries.size());
  RAT::mute_g4unmute();
  for (size_t i = 0; i < secondaries.size(); i++) {
    G4Track *secondary = secondaries[i];
    rv->AddSecondary(secondary);
  }

  return rv;
}

}  // namespace RAT
