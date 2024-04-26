#ifndef __RAT_GeoLoftFactory__
#define __RAT_GeoLoftFactory__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoLoftFactory : public GeoSolidFactory {
 public:
  GeoLoftFactory() : GeoSolidFactory("loft"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
