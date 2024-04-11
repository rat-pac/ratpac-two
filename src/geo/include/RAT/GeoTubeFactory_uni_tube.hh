#ifndef __RAT_GeoTubeFactory_uni_tube__
#define __RAT_GeoTubeFactory_uni_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_uni_tube : public GeoSolidFactory {
 public:
  GeoTubeFactory_uni_tube() : GeoSolidFactory("tube+tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
