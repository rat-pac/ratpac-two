#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4TwoVector.hh>
#include <G4UnionSolid.hh>
#include <RAT/GeoTubeFactory_uni_tube.hh>

namespace RAT {

G4VSolid *GeoTubeFactory_uni_tube::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  std::string name_tubs_base("other_buffer");
  std::string name_tubs("tube_buffer");
  G4double r_max = table->GetD("r_max") * CLHEP::mm;
  G4double other_r_max = table->GetD("other_r_max") * CLHEP::mm;
  G4double size_z = table->GetD("size_z") * CLHEP::mm;
  G4double other_size_z = table->GetD("other_size_z") * CLHEP::mm;
  const std::vector<double> &rel_xyz = table->GetDArray("rel_position");
  G4ThreeVector Rel_Trans(rel_xyz[0] * CLHEP::mm, rel_xyz[1] * CLHEP::mm, rel_xyz[2] * CLHEP::mm);

  // Optional parameters
  G4double r_min = 0.0;
  try {
    r_min = table->GetD("r_min") * CLHEP::mm;
  } catch (DBNotFoundError &e) {
  };
  G4double other_r_min = 0.0;
  try {
    other_r_min = table->GetD("other_r_min") * CLHEP::mm;
  } catch (DBNotFoundError &e) {
  };
  G4double phi_start = 0.0;
  try {
    phi_start = table->GetD("phi_start") * CLHEP::deg;
  } catch (DBNotFoundError &e) {
  };
  G4double phi_delta = CLHEP::twopi;
  try {
    phi_delta = table->GetD("phi_delta") * CLHEP::deg;
  } catch (DBNotFoundError &e) {
  };

  G4Tubs *other_cyl =
      new G4Tubs(volume_name + "_" + name_tubs_base, other_r_min, other_r_max, other_size_z, phi_start, phi_delta);
  G4Tubs *cyl = new G4Tubs(volume_name + "_" + name_tubs, r_min, r_max, size_z, phi_start, phi_delta);
  return new G4UnionSolid(volume_name, cyl, other_cyl, 0, Rel_Trans);
}

}  // namespace RAT
