#ifndef __RAT_GeoBoxFactory_uni_box__
#define __RAT_GeoBoxFactory_uni_box__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoBoxFactory_uni_box : public GeoSolidFactory {
 public:
  GeoBoxFactory_uni_box() : GeoSolidFactory("box+box"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
