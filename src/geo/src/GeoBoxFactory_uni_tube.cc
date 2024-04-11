#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4TwoVector.hh>
#include <G4UnionSolid.hh>
#include <RAT/GeoBoxFactory_uni_tube.hh>

namespace RAT {

G4VSolid *GeoBoxFactory_uni_tube::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  std::string name_box("box_buffer"), other_name_tubs("other_tube_buffer");
  const std::vector<double> &size = table->GetDArray("size");
  G4double r_max = table->GetD("r_max") * CLHEP::mm;
  G4double size_z = table->GetD("size_z") * CLHEP::mm;
  const std::vector<double> &rel_xyz = table->GetDArray("rel_position");
  G4ThreeVector Rel_Trans(rel_xyz[0] * CLHEP::mm, rel_xyz[1] * CLHEP::mm, rel_xyz[2] * CLHEP::mm);

  // Optional parameters
  G4double r_min = 0.0;
  try {
    r_min = table->GetD("r_min") * CLHEP::mm;
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

  G4Tubs *cyl = new G4Tubs(volume_name + "_" + other_name_tubs, r_min, r_max, size_z, phi_start, phi_delta);
  G4Box *box = new G4Box(volume_name + "_" + name_box, size[0] * CLHEP::mm, size[1] * CLHEP::mm, size[2] * CLHEP::mm);
  return new G4UnionSolid(volume_name, box, cyl, 0, Rel_Trans);
}

}  // namespace RAT
