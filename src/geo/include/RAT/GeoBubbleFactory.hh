#ifndef __RAT_GeoBubbleFactory__
#define __RAT_GeoBubbleFactory__

#include <G4VPhysicalVolume.hh>
#include <RAT/GeoFactory.hh>

namespace RAT {
/// \brief Makes and array of water boxes.
///
/// Makes an array of water boxes
class GeoBubbleFactory : public GeoFactory {
 public:
  GeoBubbleFactory() : GeoFactory("bubble"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
