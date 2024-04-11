#ifndef __RAT_GeoTubeFactory_int_box__
#define __RAT_GeoTubeFactory_int_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_int_box : public GeoSolidFactory {
 public:
  GeoTubeFactory_int_box() : GeoSolidFactory("tube&&box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
