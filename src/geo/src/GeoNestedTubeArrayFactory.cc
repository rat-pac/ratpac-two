#include <G4Orb.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <RAT/GeoNestedTubeArrayFactory.hh>
#include <RAT/Log.hh>
#include <vector>

namespace RAT {

G4VPhysicalVolume *GeoNestedTubeArrayFactory::Construct(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();

  info << "GeoNestedTubeArrayFactory: Constructing volume " + volume_name << newline;

  // TODO: all the parameters below are ignored and only the table parameters are used. For wider usability the above
  // parameters should be included in the array construction

  // Optional parameters
  /*
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

  // can cut out a spherical region from all the solids of
  // radius sphere_cut_r.
  // requires that rescale_r be std::set.
  G4double s_cut_r = -1.0;
  try {
    s_cut_r = table->GetD("sphere_cut_r") * CLHEP::mm;
  } catch (DBNotFoundError &e) {
  };

  // can rescale Solid radius from mother volume center for
  // case where Solids have spherical layout symmetry
  G4double rescale_r = -1.0;
  try {
    rescale_r = table->GetD("rescale_radius") * CLHEP::mm;
  } catch (DBNotFoundError &e) {
  };

  int preflip = 0;
  try {
    preflip = table->GetI("preflip");
  } catch (DBNotFoundError &e) {
  };
  */

  // End optional parameters

  /*
  if ((s_cut_r > 0) && (rescale_r > 0)) {
    G4VSolid *sphere_cutter = new G4Orb("temp_sphere", s_cut_r);  // This is the cut out piece

    G4RotationMatrix *sphererot = new G4RotationMatrix();

    G4ThreeVector spherepos(0.0, 0.0, -1 * rescale_r);
  }

  if (preflip) {
    G4RotationMatrix *fliprot = new G4RotationMatrix(G4ThreeVector(1, 0, 0), CLHEP::pi);
  }
  */

  return GeoNestedSolidArrayFactoryBase::Construct(table);
}

}  // namespace RAT
