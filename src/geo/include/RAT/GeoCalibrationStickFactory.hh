#ifndef __RAT_GeoCalibrationStickFactory__
#define __RAT_GeoCalibrationStickFactory__

#include <G4OpticalSurface.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VisAttributes.hh>
#include <RAT/GeoFactory.hh>
#include <vector>

namespace RAT {
class GeoCalibrationStickFactory : public GeoFactory {
 public:
  GeoCalibrationStickFactory() : GeoFactory("CalibrationStick"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);

 protected:
  static G4VisAttributes *GetColor(std::vector<double> color);
  void SetVis(G4LogicalVolume *volume, std::vector<double> color);
  G4OpticalSurface *GetSurface(std::string surface_name);
};

}  // namespace RAT

#endif  // __RAT_GeoCalibrationStickFactory__
