#ifndef __RAT_GeoTubeFactory_sub_tube__
#define __RAT_GeoTubeFactory_sub_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_sub_tube : public GeoSolidFactory {
 public:
  GeoTubeFactory_sub_tube() : GeoSolidFactory("tube-tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
