#include "RAT/GeoLoftFactory.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <G4SubtractionSolid.hh>
#include <G4TessellatedSolid.hh>
#include <G4TriangularFacet.hh>
#include <utility>

namespace RAT {

G4VSolid* GeoLoftFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();

  std::string poly_table_name = table->GetS("poly_table");
  DBLinkPtr lpoly_table = DB::Get()->GetLink(poly_table_name);

  // We assume to build bottom-to-up, base at z=0
  // NOTE: Maybe we can have extend to have multiple heights and xN,yN's
  const std::vector<G4double> z = lpoly_table->GetDArray("z");
  const G4double th = lpoly_table->GetD("thickness");

  const std::vector<G4double>& x1 = lpoly_table->GetDArray("x1");
  const std::vector<G4double>& y1 = lpoly_table->GetDArray("y1");
  const std::vector<G4double>& x2 = lpoly_table->GetDArray("x2");
  const std::vector<G4double>& y2 = lpoly_table->GetDArray("y2");

  // Number of points has to be same
  const size_t n = x1.size();
  assert(n >= 3);
  assert(n == y1.size());
  assert(n == x2.size());
  assert(n == y2.size());

  G4TessellatedSolid* solid = new G4TessellatedSolid(volume_name);

  const double z1 = z[0] * CLHEP::mm, z2 = z[1] * CLHEP::mm;
  const G4double dz = th * CLHEP::mm; // FIXME: to be calculated again
  for (size_t i = 0; i < n; ++i) {
    const int j = (i + 1) % n;

    // Build the outer-wall
    const G4ThreeVector voA(x1[i] * CLHEP::mm, y1[i] * CLHEP::mm, z1+dz);
    const G4ThreeVector voB(x1[j] * CLHEP::mm, y1[j] * CLHEP::mm, z1+dz);
    const G4ThreeVector voC(x2[j] * CLHEP::mm, y2[j] * CLHEP::mm, z2+dz);
    const G4ThreeVector voD(x2[i] * CLHEP::mm, y2[i] * CLHEP::mm, z2+dz);

    G4TriangularFacet* facetO1 = new G4TriangularFacet(voA, voB, voC, ABSOLUTE);
    G4TriangularFacet* facetO2 = new G4TriangularFacet(voC, voD, voA, ABSOLUTE);
    solid->AddFacet(facetO1);
    solid->AddFacet(facetO2);

    // Build the inner-wall
    const G4ThreeVector viA(x1[i] * CLHEP::mm, y1[i] * CLHEP::mm, z1);
    const G4ThreeVector viB(x1[j] * CLHEP::mm, y1[j] * CLHEP::mm, z1);
    const G4ThreeVector viC(x2[j] * CLHEP::mm, y2[j] * CLHEP::mm, z2);
    const G4ThreeVector viD(x2[i] * CLHEP::mm, y2[i] * CLHEP::mm, z2);

    G4TriangularFacet* facetI1 = new G4TriangularFacet(viD, viC, viA, ABSOLUTE);
    G4TriangularFacet* facetI2 = new G4TriangularFacet(viC, viB, viA, ABSOLUTE);
    solid->AddFacet(facetI1);
    solid->AddFacet(facetI2);

    // Finish bottom rim
    G4TriangularFacet* facetB1 = new G4TriangularFacet(voA, viA, viB, ABSOLUTE);
    G4TriangularFacet* facetB2 = new G4TriangularFacet(viB, voB, voA, ABSOLUTE);
    solid->AddFacet(facetB1);
    solid->AddFacet(facetB2);

    // Finish upper rim
    G4TriangularFacet* facetT1 = new G4TriangularFacet(voD, voC, viC, ABSOLUTE);
    G4TriangularFacet* facetT2 = new G4TriangularFacet(viC, viD, voD, ABSOLUTE);
    solid->AddFacet(facetT1);
    solid->AddFacet(facetT2);
  }

  solid->SetSolidClosed(true);
  return solid;
}

}  // namespace RAT
