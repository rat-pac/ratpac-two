#ifndef __RAT_GeoBoxFactory_int_box__
#define __RAT_GeoBoxFactory_int_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_int_box : public GeoSolidFactory {
 public:
  GeoBoxFactory_int_box() : GeoSolidFactory("box&&box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
