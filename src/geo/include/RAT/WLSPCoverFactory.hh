#ifndef __RAT_WLSPCoverFactory__
#define __RAT_WLSPCoverFactory__

#include <RAT/GeoSolidArrayFactoryBase.hh>
#include <G4VSolid.hh>

namespace RAT {
 class WLSPCoverFactory : public GeoSolidArrayFactoryBase {
 public:
   WLSPCoverFactory() : GeoSolidArrayFactoryBase("wlsp_cover") {};
   using GeoSolidArrayFactoryBase::Construct;
   virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
 };
  
} // namespace RAT

#endif
