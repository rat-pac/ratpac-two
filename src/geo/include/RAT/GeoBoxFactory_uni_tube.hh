#ifndef __RAT_GeoBoxFactory_uni_tube__
#define __RAT_GeoBoxFactory_uni_tube__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_uni_tube : public GeoSolidFactory {
 public:
  GeoBoxFactory_uni_tube() : GeoSolidFactory("box+tube"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
