#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4ExtrudedSolid.hh>
#include <G4SubtractionSolid.hh>
#include <G4TwoVector.hh>
#include <RAT/GeoGenericPolygonFactory.hh>
#include <RAT/PolygonOrientation.hh>

#include <vector>
#include <string>

namespace RAT {

G4VSolid *GeoGenericPolygonFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  const G4double size_z = table->GetD("size_z") * CLHEP::mm;  // half thickness of plate

  // Define the base polygon verticies
  std::vector<G4TwoVector> polygon;
  std::string poly_table_name = table->GetS("poly_table");
  DBLinkPtr lpoly_table = DB::Get()->GetLink(poly_table_name);
  const std::vector<G4double> &vertex_pnts_x = lpoly_table->GetDArray("x");
  const std::vector<G4double> &vertex_pnts_y = lpoly_table->GetDArray("y");
  for ( G4int i = 0, n = G4int(vertex_pnts_x.size()); i < n; ++i ) {
    polygon.push_back(G4TwoVector(vertex_pnts_x[i] * CLHEP::mm,
                                  vertex_pnts_y[i] * CLHEP::mm));
  }

  // Define polygons to subtract fro the base polygon
  std::vector<std::vector<G4TwoVector>> subpolygons;
  const int n_subpolygon = table->GetI("n_subpolygon");
  for ( int i=1; i<=n_subpolygon; ++i ) {
    //const std::string subpolygon_link_name = std::format("subpolygon_table_{}", i);
    std::string subpolygon_link_name = std::string("subpolygon_table_")+std::to_string(i);
    std::string subpolygon_table_name = table->GetS(subpolygon_link_name);
    DBLinkPtr table = DB::Get()->GetLink(subpolygon_table_name);
    const std::vector<G4double>& pnts_x = table->GetDArray("x");
    const std::vector<G4double>& pnts_y = table->GetDArray("y");
    std::vector<G4TwoVector> subpolygon;
    for ( G4int i = 0, n = G4int(pnts_x.size()); i < n; ++i) {
      subpolygon.push_back(G4TwoVector(pnts_x[i] * CLHEP::mm,
                                    pnts_y[i] * CLHEP::mm));
    }
    subpolygons.push_back(subpolygon);
  }

  // Build the base solid
  G4TwoVector zero_offset(0,0);

  G4VSolid* base_solid = new G4ExtrudedSolid(volume_name, polygon, size_z/2, zero_offset, 1, zero_offset, 1);

  // Subtract polygons
  for ( auto subpoly : subpolygons ) {
    // Note: the height is changed to size_z/2 -> size_z to clearly cut out
    G4VSolid* sub_solid = new G4ExtrudedSolid(volume_name + "_sub_solid", subpoly, size_z, zero_offset, 1, zero_offset, 1);
    base_solid = new G4SubtractionSolid(volume_name, base_solid, sub_solid, 0, G4ThreeVector(0.0, 0.0, 0.0));
  }

  return base_solid;
}

}  // namespace RAT
