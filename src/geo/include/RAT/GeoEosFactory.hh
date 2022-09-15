#ifndef __RAT_GeoEosFactory__
#define __RAT_GeoEosFactory__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoEosFactory : public GeoSolidFactory {
 public:
  GeoEosFactory() : GeoSolidFactory("eos"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
