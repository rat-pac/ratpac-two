#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <G4ThreeVector.hh>
#include <G4TwoVector.hh>
#include <G4UnionSolid.hh>
#include <RAT/GeoBoxFactory_uni_box.hh>

namespace RAT {

G4VSolid *GeoBoxFactory_uni_box::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  std::string name_box("box_buffer");
  std::string other_name_box("other_box_buffer");
  const std::vector<double> &size = table->GetDArray("size");
  const std::vector<double> &other_size = table->GetDArray("other_size");
  const std::vector<double> &rel_xyz = table->GetDArray("rel_position");
  G4ThreeVector Rel_Trans(rel_xyz[0] * CLHEP::mm, rel_xyz[1] * CLHEP::mm, rel_xyz[2] * CLHEP::mm);

  G4Box *box = new G4Box(volume_name + "_" + name_box, size[0] * CLHEP::mm, size[1] * CLHEP::mm, size[2] * CLHEP::mm);
  G4Box *other_box = new G4Box(volume_name + "_" + other_name_box, other_size[0] * CLHEP::mm, other_size[1] * CLHEP::mm,
                               other_size[2] * CLHEP::mm);

  return new G4UnionSolid(volume_name, box, other_box, 0, Rel_Trans);
}
}  // namespace RAT

// namespace RAT