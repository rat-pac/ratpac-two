#ifndef __RAT_GeoWatchmanShieldFactory__
#define __RAT_GeoWatchmanShieldFactory__

#include <G4OpticalSurface.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VisAttributes.hh>
#include <RAT/GeoFactory.hh>

namespace RAT {
class GeoWatchmanShieldFactory : public GeoFactory {
 public:
  GeoWatchmanShieldFactory() : GeoFactory("watchmanshield"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);

 protected:
  G4VisAttributes *GetVisAttributes(DBLinkPtr table);
  G4OpticalSurface *GetSurface(std::string surface_name);
};

}  // namespace RAT

#endif
