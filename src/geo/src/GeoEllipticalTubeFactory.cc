#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4EllipticalTube.hh>
#include <RAT/GeoEllipticalTubeFactory.hh>

namespace RAT {

G4VSolid *GeoEllipticalTubeFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  G4double r_x = table->GetD("r_x") * CLHEP::mm;
  G4double r_y = table->GetD("r_y") * CLHEP::mm;
  G4double size_z = table->GetD("size_z") * CLHEP::mm;

  return new G4EllipticalTube(volume_name, r_x, r_y, size_z);
}

}  // namespace RAT
