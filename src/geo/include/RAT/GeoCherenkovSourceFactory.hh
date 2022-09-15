////////////////////////////////////////////////////////////////////////
/// \class RAT::GeoCherenkovSourceFactory
///
/// \brief Geometry for the Cherenkov Source being developed at Berkeley
///
/// \author Benjamin Land
///
/// REVISION HISTORY:\n
/// 	17/06/14 : B. Land - Folded development version into RAT \n
///
/// \details
/// 	Constructs the Cherenkov source geometry for the actual source
///     being developed at Berkeley. (work in progress)
///
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_GeoCherenkovSourceFactory__
#define __RAT_GeoCherenkovSourceFactory__

#include <G4OpticalSurface.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VisAttributes.hh>
#include <RAT/GeoFactory.hh>
#include <vector>

namespace RAT {
class GeoCherenkovSourceFactory : public GeoFactory {
 public:
  GeoCherenkovSourceFactory() : GeoFactory("CherenkovSource"){};
  virtual G4VPhysicalVolume *Construct(DBLinkPtr table);

 protected:
  static G4VisAttributes *GetColor(std::vector<double> color);
  G4OpticalSurface *GetSurface(std::string surface_name);
};

}  // namespace RAT

#endif  // __RAT_GeoCherenkovSourceFactory__
