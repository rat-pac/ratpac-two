#ifndef __RAT_GeoBoxFactory_sub_tube__
#define __RAT_GeoBoxFactory_sub_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_sub_tube : public GeoSolidFactory {
 public:
  GeoBoxFactory_sub_tube() : GeoSolidFactory("box-tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
