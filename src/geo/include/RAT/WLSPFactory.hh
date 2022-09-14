#ifndef __RAT_WLSPFactory__
#define __RAT_WLSPFactory__

#include <G4VSolid.hh>
#include <RAT/GeoSolidArrayFactoryBase.hh>

namespace RAT {
class WLSPFactory : public GeoSolidArrayFactoryBase {
 public:
  WLSPFactory() : GeoSolidArrayFactoryBase("wlsp"){};
  using GeoSolidArrayFactoryBase::Construct;
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
