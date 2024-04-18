#include "RAT/GeoLoftFactory.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <G4QuadrangularFacet.hh>
#include <G4SubtractionSolid.hh>
#include <G4TessellatedSolid.hh>

namespace RAT {

G4VSolid* GeoLoftFactory::ConstructSolid(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();

  // We assume to build bottom-to-up, base at z=0
  // NOTE: Maybe we can have extend to have multiple heights and xN,yN's
  const std::vector<double> z = table->GetDArray("z");
  const double thickness = table->GetD("thickness");

  const std::vector<double>& x1 = table->GetDArray("x1");
  const std::vector<double>& y1 = table->GetDArray("y1");

  const std::vector<double>& x2 = table->GetDArray("x2");
  const std::vector<double>& y2 = table->GetDArray("y2");

  // Number of points has to be same
  const size_t n = x1.size();
  assert(n == x2.size());
  assert(n == y1.size());
  assert(n == y2.size());

  // Build outer loft-shape
  const double z1 = z[0] * CLHEP::mm, z2 = z[1] * CLHEP::mm;
  G4TessellatedSolid* base_solid = new G4TessellatedSolid(volume_name);
  G4VSolid* solid = nullptr;

  double xmin = 0, xmax = 0;  // to set the scale value
  for (size_t i = 0; i < n; ++i) {
    if (i == 0 or xmin > x1[i]) xmin = x1[i];
    if (i == 0 or xmax < x1[i]) xmax = x1[i];
    const int j = (i + 1) % n;

    const G4ThreeVector pA(x1[i] * CLHEP::mm, y1[i] * CLHEP::mm, z1);
    const G4ThreeVector pB(x1[j] * CLHEP::mm, y1[j] * CLHEP::mm, z1);
    const G4ThreeVector pC(x2[j] * CLHEP::mm, y2[j] * CLHEP::mm, z2);
    const G4ThreeVector pD(x2[i] * CLHEP::mm, y2[i] * CLHEP::mm, z2);

    G4QuadrangularFacet* facet = new G4QuadrangularFacet(pA, pB, pC, pD, ABSOLUTE);
    base_solid->AddFacet(facet);
  }

  // Build inner loft-shape
  if (thickness <= 0) {
    solid = base_solid;
  } else {
    const double scale = 1 - 2 * thickness / (xmax - xmin);
    G4TessellatedSolid* sub_solid = new G4TessellatedSolid(volume_name + "_sub");

    for (size_t i = 0; i < n; ++i) {
      const int j = (i + 1) % n;

      const G4ThreeVector pA(x1[i] * scale * CLHEP::mm, y1[i] * scale * CLHEP::mm, z1);
      const G4ThreeVector pB(x1[j] * scale * CLHEP::mm, y1[j] * scale * CLHEP::mm, z1);
      const G4ThreeVector pC(x2[j] * scale * CLHEP::mm, y2[j] * scale * CLHEP::mm, z2);
      const G4ThreeVector pD(x2[i] * scale * CLHEP::mm, y2[i] * scale * CLHEP::mm, z2);

      G4QuadrangularFacet* facet = new G4QuadrangularFacet(pA, pB, pC, pD, ABSOLUTE);
      sub_solid->AddFacet(facet);
    }

    solid = new G4SubtractionSolid(volume_name, base_solid, sub_solid);
  }

  return solid;
}

}  // namespace RAT
