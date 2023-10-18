#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <RAT/PhotonThinning.hh>

namespace RAT {

double PhotonThinning::fThinFactor;

double PhotonThinning::fCherenkovThinningFactor;
double PhotonThinning::fScintillationThinningFactor;

double PhotonThinning::fCherenkovLowerWavelengthThreshold;
double PhotonThinning::fScintillationLowerWavelengthThreshold;

double PhotonThinning::fCherenkovUpperWavelengthThreshold;
double PhotonThinning::fScintillationUpperWavelengthThreshold;

void PhotonThinning::Init() {
  SetFactor(DB::Get()->GetLink("MC")->GetD("thin_factor"));

  SetCherenkovThinningFactor(DB::Get()->GetLink("PHOTON_THINNING")->GetD("cherenkov_thinning_factor"));
  SetScintillationThinningFactor(DB::Get()->GetLink("PHOTON_THINNING")->GetD("scintillation_thinning_factor"));

  SetCherenkovLowerWavelengthThreshold(
      DB::Get()->GetLink("PHOTON_THINNING")->GetD("cherenkov_lower_wavelength_threshold"));
  SetScintillationLowerWavelengthThreshold(
      DB::Get()->GetLink("PHOTON_THINNING")->GetD("scintillation_lower_wavelength_threshold"));

  SetCherenkovUpperWavelengthThreshold(
      DB::Get()->GetLink("PHOTON_THINNING")->GetD("cherenkov_upper_wavelength_threshold"));
  SetScintillationUpperWavelengthThreshold(
      DB::Get()->GetLink("PHOTON_THINNING")->GetD("scintillation_upper_wavelength_threshold"));
}

void PhotonThinning::SetFactor(double factor) {
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set photon thinning factor %f which is less than 1.0", factor));
  }
  fThinFactor = factor;
}

void PhotonThinning::SetCherenkovThinningFactor(double factor) {
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set Cherenkov thinning factor %f which is less than 1.0", factor));
  }
  fCherenkovThinningFactor = factor;
}

void PhotonThinning::SetScintillationThinningFactor(double factor) {
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set Scintillation thinning factor %f which is less than 1.0", factor));
  }
  fScintillationThinningFactor = factor;
}

void PhotonThinning::SetCherenkovLowerWavelengthThreshold(double thresh) {
  if (thresh < 0.0) {
    Log::Die(dformat("Cannot set Cherenkov lower wavelength threshold %f which is less than 0.0", thresh));
  }
  fCherenkovLowerWavelengthThreshold = thresh;
}

void PhotonThinning::SetScintillationLowerWavelengthThreshold(double thresh) {
  if (thresh < 0.0) {
    Log::Die(dformat("Cannot set Scintillation lower wavelength threshold %f which is less than 0.0", thresh));
  }
  fScintillationLowerWavelengthThreshold = thresh;
}

void PhotonThinning::SetCherenkovUpperWavelengthThreshold(double thresh) {
  if (thresh < 0.0) {
    Log::Die(dformat("Cannot set Cherenkov upper wavelength threshold %f which is less than 0.0", thresh));
  }
  fCherenkovUpperWavelengthThreshold = thresh;
}

void PhotonThinning::SetScintillationUpperWavelengthThreshold(double thresh) {
  if (thresh < 0.0) {
    Log::Die(dformat("Cannot set Scintillation upper wavelength threshold %f which is less than 0.0", thresh));
  }
  fScintillationUpperWavelengthThreshold = thresh;
}

}  // namespace RAT
