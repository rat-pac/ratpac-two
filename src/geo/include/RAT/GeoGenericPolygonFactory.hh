#ifndef __RAT_GeoGenericPolygonFactory__
#define __RAT_GeoGenericPolygonFactory__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoGenericPolygonFactory : public GeoSolidFactory {
 public:
  GeoGenericPolygonFactory() : GeoSolidFactory("genpoly"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
