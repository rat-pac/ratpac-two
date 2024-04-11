#ifndef __RAT_GeoBoxFactory_int_tube__
#define __RAT_GeoBoxFactory_int_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_int_tube : public GeoSolidFactory {
 public:
  GeoBoxFactory_int_tube() : GeoSolidFactory("box&&tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
