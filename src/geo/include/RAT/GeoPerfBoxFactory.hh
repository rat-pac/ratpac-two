/* Creates a rectangular solid with a set of holes punched out.
 */

#ifndef __RAT_GeoPerfBoxFactory__
#define __RAT_GeoPerfBoxFactory__

#include <G4VSolid.hh>
#include <RAT/DB.hh>
#include <RAT/GeoSolidFactory.hh>

namespace RAT {
class GeoPerfBoxFactory : public GeoSolidFactory {
 public:
  GeoPerfBoxFactory() : GeoSolidFactory("perfbox"){};
  virtual G4VSolid *ConstructSolid(DBLinkPtr table);
};

}  // namespace RAT

#endif
