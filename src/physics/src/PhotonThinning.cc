#include <RAT/DB.hh>
#include <RAT/PhotonThinning.hh>
#include <RAT/Log.hh>

namespace RAT {


double PhotonThinning::fThinFactor;
double PhotonThinning::fCherenkovThinningFactor;
double PhotonThinning::fScintillationThinningFactor;

void PhotonThinning::Init()
{
  SetFactor(DB::Get()->GetLink("MC")->GetD("thin_factor"));
  SetCherenkovThinningFactor(DB::Get()->GetLink("PHOTON_THINNING")->GetD("cherenkov_thinning_factor"));
  SetScintillationThinningFactor(DB::Get()->GetLink("PHOTON_THINNING")->GetD("scintillation_thinning_factor"));
}

void PhotonThinning::SetFactor(double factor)
{
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set photon thinning factor %f which is less than 1.0", factor));
  }
  fThinFactor = factor;
}

void PhotonThinning::SetCherenkovThinningFactor(double factor)
{
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set Cherenkov thinning factor %f which is less than 1.0", factor));
  }
  fCherenkovThinningFactor = factor;
}

void PhotonThinning::SetScintillationThinningFactor(double factor)
{
  if (factor < 1.0) {
    Log::Die(dformat("Cannot set Scintillation thinning factor %f which is less than 1.0", factor));
  }
  fScintillationThinningFactor = factor;
}

} // namespace RAT
