#ifndef __RAT_GeoBoxFactory_sub_box__
#define __RAT_GeoBoxFactory_sub_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_sub_box : public GeoSolidFactory {
 public:
  GeoBoxFactory_sub_box() : GeoSolidFactory("box-box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
