#ifndef __RAT_WLSPFactory__
#define __RAT_WLSPFactory__

#include <RAT/GeoSolidArrayFactoryBase.hh>
#include <G4VSolid.hh>

namespace RAT {
 class WLSPFactory : public GeoSolidArrayFactoryBase {
 public:
   WLSPFactory() : GeoSolidArrayFactoryBase("wlsp") {};
   using GeoSolidArrayFactoryBase::Construct;
   virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
 };
  
} // namespace RAT

#endif
