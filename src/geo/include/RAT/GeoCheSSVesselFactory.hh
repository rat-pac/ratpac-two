#ifndef __RAT_GeoCheSSVesselFactory__
#define __RAT_GeoCheSSVesselFactory__

#include <RAT/GeoSolidFactory.hh>

namespace RAT {
 class GeoCheSSVesselFactory : public GeoSolidFactory {
 public:
   GeoCheSSVesselFactory() : GeoSolidFactory("CheSSVessel") {};
   virtual G4VSolid *ConstructSolid(DBLinkPtr table);
 };

} // namespace RAT

#endif
