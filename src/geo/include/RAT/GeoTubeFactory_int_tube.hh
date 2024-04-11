#ifndef __RAT_GeoTubeFactory_int_tube__
#define __RAT_GeoTubeFactory_int_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_int_tube : public GeoSolidFactory {
 public:
  GeoTubeFactory_int_tube() : GeoSolidFactory("tube&&tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
