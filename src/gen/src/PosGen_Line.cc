#include <RAT/GLG4StringUtil.hh>
#include <RAT/PosGen_Line.hh>
#include <RAT/Log.hh>
#include <Randomize.hh>
#include <sstream>

namespace RAT {

PosGen_Line::PosGen_Line(const char *arg_dbname) : GLG4PosGen(arg_dbname), fPoint1(0., 0., 0.), fPoint2(0., 0., 0.) {}

void PosGen_Line::GeneratePosition(G4ThreeVector &argResult) {
  G4ThreeVector diff = fPoint2 - fPoint1;
  G4double scale = G4UniformRand();

  argResult = fPoint1 + scale * diff;
}

void PosGen_Line::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this GLG4PosGen_Line:" << newline
              << " \"" << GetState() << "\"" << newline << newline;
    info << "Format of argument to PosGen_Line::SetState: " << newline
              <<  " \"x1_mm y1_mm z1_mm x2_mm y2_mm z2_mm\"" << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double x1, y1, z1, x2, y2, z2;
  is >> x1 >> y1 >> z1 >> x2 >> y2 >> z2;
  if (is.fail()) {
    warn << "PosGen_Line::SetState: Could not parse six doubles from input string" << newline;
    return;
  }
  fPoint1 = G4ThreeVector(x1, y1, z1);
  fPoint2 = G4ThreeVector(x2, y2, z2);
}

G4String PosGen_Line::GetState() const {
  return util_dformat("%ld\t%ld\t%ld -> %ld\t%ld\t%ld", fPoint1.x(), fPoint1.y(), fPoint1.z(), fPoint2.x(), fPoint2.y(),
                      fPoint2.z());
}

}  // namespace RAT
