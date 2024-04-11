#ifndef __RAT_GeoTubeFactory_uni_box__
#define __RAT_GeoTubeFactory_uni_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoTubeFactory_uni_box : public GeoSolidFactory {
 public:
  GeoTubeFactory_uni_box() : GeoSolidFactory("tube+box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
