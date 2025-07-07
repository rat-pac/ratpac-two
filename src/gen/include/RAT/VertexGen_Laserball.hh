#ifndef __RAT_VertexGen_Laserball__
#define __RAT_VertexGen_Laserball__

#include <CLHEP/Random/RandGeneral.h>

#include <RAT/GLG4VertexGen.hh>

namespace RAT {

class VertexGen_Laserball : public GLG4VertexGen {
 public:
  VertexGen_Laserball(const char *arg_dbname = "laserball");
  virtual void GeneratePrimaryVertex(G4Event *argEvent, G4ThreeVector &dx, G4double dt);
  /** State format "num_photons wavelength[nm]nm" */
  virtual void SetState(G4String newValues);
  virtual G4String GetState();
  double pickWavelength(const std::vector<double> &values, const std::vector<double> &probs);

 private:
  G4ParticleDefinition *fOpticalPhoton;
  size_t fNumPhotons;
  double fExpTime;
  std::string fWavelengthIndex;
};

}  // namespace RAT

#endif
