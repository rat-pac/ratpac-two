#ifndef __RAT_GeoEllipticalTubeFactory__
#define __RAT_GeoEllipticalTubeFactory__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoEllipticalTubeFactory : public GeoSolidFactory {
 public:
  GeoEllipticalTubeFactory() : GeoSolidFactory("ellipticaltube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
