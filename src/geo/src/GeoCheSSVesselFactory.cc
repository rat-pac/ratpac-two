#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <RAT/GeoCheSSVesselFactory.hh>

namespace RAT {

G4VSolid *GeoCheSSVesselFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  const double r_min = 0.;
  const double r_max = table->GetD("r_max");
  const double size_z = table->GetD("size_z");
  const bool doCavity = table->GetZ("cavity");
  const int pmts = table->GetI("pmts");
  G4Tubs *cup =
      new G4Tubs("cup", r_min * CLHEP::mm, r_max * CLHEP::mm, (size_z + 3.18 / 2.) * CLHEP::mm, 0., CLHEP::twopi);
  G4Box *flat = new G4Box("flat", 5.0 * CLHEP::mm, size_z * CLHEP::mm, size_z * CLHEP::mm);
  G4Box *flat_pmt = new G4Box("flat_pmt", 35.0 * CLHEP::mm, 15.0 * CLHEP::mm, size_z * CLHEP::mm);
  G4Box *cavity = new G4Box("cavity", 3.0 * CLHEP::mm, 50. * CLHEP::mm, 50. * CLHEP::mm);

  G4ThreeVector *trans = new G4ThreeVector(0., 0., (size_z + 3.18 / 2.) * CLHEP::mm);
  G4RotationMatrix *rotation = new G4RotationMatrix();
  G4Transform3D *transf = new G4Transform3D(*rotation, *trans);

  // First flat face
  G4BooleanSolid *firstVolume = NULL;
  if (doCavity) {
    // Cavity
    trans = new G4ThreeVector(r_max * CLHEP::mm, 38., -3.18 / 2.);
    transf = new G4Transform3D(*rotation, *trans);
    trans->rotateZ(2 * atan(size_z / r_max) * CLHEP::rad);
    rotation->rotateZ(2 * atan(size_z / r_max) * CLHEP::rad);
    transf = new G4Transform3D(*rotation, *trans);
    firstVolume = new G4SubtractionSolid("firstVol", cup, cavity, *transf);
  } else {
    trans = new G4ThreeVector((r_max - 5.0) * CLHEP::mm, 0., -3.18 / 2.);
    trans->rotateZ(2 * atan(size_z / r_max) * CLHEP::rad);
    rotation->rotateZ(2 * atan(size_z / r_max) * CLHEP::rad);
    transf = new G4Transform3D(*rotation, *trans);
    firstVolume = new G4UnionSolid("firstVol", cup, flat, *transf);
  }

  // Second flat face
  trans = new G4ThreeVector((r_max - 5.0) * CLHEP::mm, 0., -3.18 / 2.);
  rotation->rotateZ(-2 * atan(size_z / r_max) * CLHEP::rad);
  transf = new G4Transform3D(*rotation, *trans);
  G4UnionSolid *unionVolume0 = NULL;
  if (pmts == 0)
    unionVolume0 = new G4UnionSolid("union0", firstVolume, flat, *transf);
  else if (pmts == 1)
    unionVolume0 = new G4UnionSolid("union0", firstVolume, flat_pmt, *transf);
  else if (pmts == 2)
    unionVolume0 = new G4UnionSolid("union0", firstVolume, flat, *transf);
  else {
    std::cout << " GeoCheSSVessel: Number of PMTs must be 0, 1 or 2" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Third flat face
  trans->rotateZ(-2 * atan(size_z / r_max) * CLHEP::rad);
  rotation->rotateZ(-2 * atan(size_z / r_max) * CLHEP::rad);
  transf = new G4Transform3D(*rotation, *trans);
  G4UnionSolid *finalVolume = NULL;
  if (pmts == 0)
    finalVolume = new G4UnionSolid(volume_name, unionVolume0, flat, *transf);
  else
    finalVolume = new G4UnionSolid(volume_name, unionVolume0, flat, *transf);

  return finalVolume;
}

}  // namespace RAT
