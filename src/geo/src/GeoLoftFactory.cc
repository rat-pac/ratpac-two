#include "RAT/GeoLoftFactory.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <G4QuadrangularFacet.hh>
#include <G4SubtractionSolid.hh>
#include <G4TessellatedSolid.hh>
#include <G4TriangularFacet.hh>
#include <utility>

namespace RAT {

G4VSolid* GeoLoftFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();

  // We assume to build bottom-to-up, base at z=0
  // NOTE: Maybe we can have extend to have multiple heights and xN,yN's
  const std::vector<double> z = table->GetDArray("z");

  const std::vector<double>& xo1 = table->GetDArray("out_x1");
  const std::vector<double>& yo1 = table->GetDArray("out_y1");
  const std::vector<double>& xo2 = table->GetDArray("out_x2");
  const std::vector<double>& yo2 = table->GetDArray("out_y2");

  const std::vector<double>& xi1 = table->GetDArray("in_x1");
  const std::vector<double>& yi1 = table->GetDArray("in_y1");
  const std::vector<double>& xi2 = table->GetDArray("in_x2");
  const std::vector<double>& yi2 = table->GetDArray("in_y2");

  // Number of points has to be same
  const size_t n = xo1.size();
  assert(n >= 3);
  assert(n == yo1.size());
  assert(n == xo2.size());
  assert(n == yo2.size());
  assert(n == xi1.size());
  assert(n == yi1.size());
  assert(n == xi2.size());
  assert(n == yi2.size());

  G4TessellatedSolid* solid = new G4TessellatedSolid(volume_name);

  const double z1 = z[0] * CLHEP::mm, z2 = z[1] * CLHEP::mm;
  for (size_t i = 0; i < n; ++i) {
    const int j = (i + 1) % n;

    // Build the outer-wall
    const G4ThreeVector voA(xo1[i] * CLHEP::mm, yo1[i] * CLHEP::mm, z1);
    const G4ThreeVector voB(xo1[j] * CLHEP::mm, yo1[j] * CLHEP::mm, z1);
    const G4ThreeVector voC(xo2[j] * CLHEP::mm, yo2[j] * CLHEP::mm, z2);
    const G4ThreeVector voD(xo2[i] * CLHEP::mm, yo2[i] * CLHEP::mm, z2);

    G4QuadrangularFacet* facetOut = new G4QuadrangularFacet(voA, voB, voC, voD, ABSOLUTE);
    solid->AddFacet(facetOut);

    // Build the inner-wall
    const G4ThreeVector viA(xi1[i] * CLHEP::mm, yi1[i] * CLHEP::mm, z1);
    const G4ThreeVector viB(xi1[j] * CLHEP::mm, yi1[j] * CLHEP::mm, z1);
    const G4ThreeVector viC(xi2[j] * CLHEP::mm, yi2[j] * CLHEP::mm, z2);
    const G4ThreeVector viD(xi2[i] * CLHEP::mm, yi2[i] * CLHEP::mm, z2);

    G4QuadrangularFacet* facetIn = new G4QuadrangularFacet(viD, viC, viB, viA, ABSOLUTE);
    solid->AddFacet(facetIn);

    // Finish bottom rim
    G4QuadrangularFacet* facetBot = new G4QuadrangularFacet(voA, viA, viB, voB, ABSOLUTE);
    solid->AddFacet(facetBot);

    // Finish upper rim
    G4QuadrangularFacet* facetTop = new G4QuadrangularFacet(voD, voC, viC, viD, ABSOLUTE);
    solid->AddFacet(facetTop);
  }

  solid->SetSolidClosed(true);
  return solid;
}

}  // namespace RAT
