#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <RAT/GeoPerfTubeFactory.hh>
#include <RAT/PMTInfoParser.hh>
#include <RAT/ToroidalPMTConstruction.hh>
#include <vector>

namespace RAT {

G4VSolid *GeoPerfTubeFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  // Find mother
  std::string mother_name = table->GetS("mother");
  G4LogicalVolume *mother = FindMother(mother_name);
  G4double r_max = table->GetD("r_max") * CLHEP::mm;    // radius of main plate
  G4double size_z = table->GetD("size_z") * CLHEP::mm;  // half thickness of plate

  G4double r_hole = table->GetD("r_hole") * CLHEP::mm;  // radius of the holes.  If this is std::set <= zero,
                                                        // then use PMTs to make the holes.

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
  // end optional parms

  G4VSolid *base_tube = new G4Tubs(volume_name,  // start with this cylinder/tube
                                   r_min, r_max, size_z, phi_start, phi_delta);

  if (r_hole > 0) {
    std::string pos_table_name = table->GetS("pos_table");
    DBLinkPtr lpos_table = DB::Get()->GetLink(pos_table_name);
    const std::vector<double> &hole_r = lpos_table->GetDArray("r");  // radial position of hole (mm)
    const std::vector<double> &hole_a = lpos_table->GetDArray("a");  // angle of the hole (in radians)

    int num_holes = hole_a.size();

    G4VSolid *hole_cutter = new G4Tubs("temp",  // This is the cut out piece
                                       0.0, r_hole, size_z * 1.01, 0.0, CLHEP::twopi);

    for (int holeID = 0; holeID < num_holes; holeID++) {
      base_tube = new G4SubtractionSolid(
          volume_name, base_tube, hole_cutter, 0,
          G4ThreeVector(hole_r[holeID] * cos(hole_a[holeID]), hole_r[holeID] * sin(hole_a[holeID]), 0.0));
    }
  } else {
    std::string pmt_table = table->GetS("pmt_table");
    DBLinkPtr lgeo_pmt = DB::Get()->GetLink("GEO", pmt_table);
    PMTInfoParser pmt_parser(lgeo_pmt, mother_name);
    DBLinkPtr lpmt_model = DB::Get()->GetLink("PMT", lgeo_pmt->GetS("pmt_model"));
    ToroidalPMTConstruction pmtConstruct(lpmt_model, mother);
    G4VSolid *pmtBody = pmtConstruct.BuildSolid("dummy");

    std::vector<G4ThreeVector> pmtloc = pmt_parser.GetPMTLocations();
    std::vector<G4ThreeVector> pmtdir = pmt_parser.GetPMTDirections();
    int max_pmts = pmtloc.size();

    const std::vector<double> &posvector = table->GetDArray("position");
    G4ThreeVector baseloc(posvector[0] * CLHEP::mm, posvector[1] * CLHEP::mm, posvector[2] * CLHEP::mm);

    for (int pmtID = 0; pmtID < max_pmts; pmtID++) {
      G4RotationMatrix pmtrot = pmt_parser.GetPMTRotation(pmtID);
      base_tube = new G4SubtractionSolid(volume_name, base_tube, pmtBody, &pmtrot, pmtloc[pmtID] - baseloc);
    }
  }

  return base_tube;
}

}  // namespace RAT
