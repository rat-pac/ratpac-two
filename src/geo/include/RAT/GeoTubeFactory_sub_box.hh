#ifndef __RAT_GeoTubeFactory_sub_box__
#define __RAT_GeoTubeFactory_sub_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_sub_box : public GeoSolidFactory {
 public:
  GeoTubeFactory_sub_box() : GeoSolidFactory("tube-box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
