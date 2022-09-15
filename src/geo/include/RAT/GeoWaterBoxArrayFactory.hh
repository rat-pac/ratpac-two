#ifndef __RAT_GeoWaterBoxArrayFactory__
#define __RAT_GeoWaterBoxArrayFactory__

#include <G4VPhysicalVolume.hh>
#include <RAT/GeoFactory.hh>

namespace RAT {
/// \brief Makes and array of water boxes.
///
/// Makes an array of water boxes
class GeoWaterBoxArrayFactory : public GeoFactory {
 public:
  GeoWaterBoxArrayFactory() : GeoFactory("waterboxarray") {}
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
