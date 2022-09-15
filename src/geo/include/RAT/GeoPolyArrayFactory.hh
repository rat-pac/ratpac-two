#ifndef __RAT_GeoPolyArrayFactory__
#define __RAT_GeoPolyArrayFactory__

#include <RAT/GeoSolidArrayFactoryBase.hh>

namespace RAT {
class GeoPolyArrayFactory : public GeoSolidArrayFactoryBase {
 public:
  GeoPolyArrayFactory() : GeoSolidArrayFactoryBase("polygonarray"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
