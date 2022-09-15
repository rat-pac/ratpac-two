#ifndef __RAT_WLSPCoverFactory__
#define __RAT_WLSPCoverFactory__

#include <G4VSolid.hh>
#include <RAT/GeoSolidArrayFactoryBase.hh>

namespace RAT {
class WLSPCoverFactory : public GeoSolidArrayFactoryBase {
 public:
  WLSPCoverFactory() : GeoSolidArrayFactoryBase("wlsp_cover"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
