#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <RAT/GeoBoxFactory.hh>

namespace RAT {

G4VSolid *GeoBoxFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  const std::vector<double> &size = table->GetDArray("size");
  return new G4Box(volume_name, size[0] * CLHEP::mm, size[1] * CLHEP::mm, size[2] * CLHEP::mm);
}

}  // namespace RAT
