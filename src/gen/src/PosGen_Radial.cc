#include <RAT/GLG4StringUtil.hh>
#include <RAT/PosGen_Radial.hh>
#include <RAT/Log.hh>
#include <Randomize.hh>
#include <sstream>

#include "TMath.h"

namespace RAT {

PosGen_Radial::PosGen_Radial(const char *arg_dbname) : GLG4PosGen(arg_dbname), fCenter(0., 0., 0.), fMaxRadius(0.) {}

void PosGen_Radial::GeneratePosition(G4ThreeVector &argResult) {
  fPoint = G4ThreeVector(0, 0, 0);
  fPoint.setRThetaPhi(G4UniformRand(), acos(2. * G4UniformRand() - 1.), (2. * G4UniformRand() - 1.) * TMath::Pi());

  argResult = fMaxRadius * fPoint + fCenter;
}

void PosGen_Radial::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this PosGen_Radial:" << newline
              << " \"" << GetState() << "\"" << newline
              << newline;
    info << "Format of argument to PosGen_Radial::SetState: " << newline
              << " \"x_mm y_mm z_mm R_mm\"" << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double x, y, z, R;
  is >> x >> y >> z >> R;
  if (is.fail()) {
    warn << "PosGen_Radial::SetState: Could not parse four doubles from input string" << newline;
    return;
  }
  fCenter = G4ThreeVector(x, y, z);
  fMaxRadius = R;
}

G4String PosGen_Radial::GetState() const {
  return util_dformat("%.3f\t%.3f\t%.3f\t%.3f", fCenter.x(), fCenter.y(), fCenter.z(), fMaxRadius);
}

}  // namespace RAT
