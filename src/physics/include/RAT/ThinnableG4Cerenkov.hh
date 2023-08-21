/**
 * @class RAT::ThinnableG4Cerenkov
 * @brief Arbitrarily thins Cherenkov photon production with no compensatory
 * side-effects
 *
 * @author Ed Callaghan <ejc3@berkeley.edu>
 *
 * @detail This class extends G4Cerenkov to allow for arbitary downscalings
 * (i.e., a factor of < 1) of the production of Cherenkov photons
 */

#ifndef __RAT_ThinnableG4Cereknov__
#define __RAT_ThinnableG4Cereknov__

#include <CLHEP/Random/Random.h>

#include <G4Cerenkov.hh>

namespace RAT {

class ThinnableG4Cerenkov : public G4Cerenkov {
 public:
  ThinnableG4Cerenkov();
  void SetThinningFactor(double);
  double GetThinningFactor();
  void SetLowerWavelengthThreshold(double);
  double GetLowerWavelengthThreshold();
  void SetUpperWavelengthThreshold(double);
  double GetUpperWavelengthThreshold();
  G4VParticleChange *PostStepDoIt(const G4Track &, const G4Step &);

 private:
  bool should_thin;
  double thinning_factor;
  double lower_wavelength_threshold;
  double upper_wavelength_threshold;
  CLHEP::HepRandom heprandom;
};

}  // namespace RAT

#endif
