#ifndef __RAT_GeoNestedSolidArrayFactoryBase__
#define __RAT_GeoNestedSolidArrayFactoryBase__

#include <RAT/DS/NestedTubeInfo.hh>
#include <RAT/GeoFactory.hh>

namespace RAT {
class GeoNestedSolidArrayFactoryBase : public GeoFactory {
 public:
  GeoNestedSolidArrayFactoryBase(const std::string &name) : GeoFactory(name) {};
  static const DS::NestedTubeInfo &GetNestedTubeInfo() { return nestedtubeinfo; }

 protected:
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);

  static DS::NestedTubeInfo nestedtubeinfo;
};

}  // namespace RAT

#endif
