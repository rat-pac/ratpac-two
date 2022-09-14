#ifndef __RATGeoBuilder__
#define __RATGeoBuilder__

#include <G4VPhysicalVolume.hh>
#include <RAT/DB.hh>
#include <string>
#include <vector>

namespace RAT {

class GeoBuilder {
 public:
  // Initialize factory with default classes
  GeoBuilder();
  // Construct all geometry from database, returns world volume
  G4VPhysicalVolume *ConstructAll(std::string geo_tablename = "GEO");

  // Construct a volume from particular table (assumes mother volume already
  // exists), returns physical volume
  G4VPhysicalVolume *Construct(DBLinkPtr table);
};

}  // namespace RAT

#endif
