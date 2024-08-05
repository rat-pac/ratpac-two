#ifndef __RAT_GeoNestedTubeArrayFactory__
#define __RAT_GeoNestedTubeArrayFactory__

#include <RAT/GeoNestedSolidArrayFactoryBase.hh>

namespace RAT {
class GeoNestedTubeArrayFactory : public GeoNestedSolidArrayFactoryBase {
 public:
  GeoNestedTubeArrayFactory() : GeoNestedSolidArrayFactoryBase("nestedtubearray"){};
  using GeoNestedSolidArrayFactoryBase::Construct;
  virtual G4VPhysicalVolume *Construct(RAT::DBLinkPtr table);
};

}  // namespace RAT

#endif
