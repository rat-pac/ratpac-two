#ifndef __RAT_GeoNestedTubeConstruction__
#define __RAT_GeoNestedTubeConstruction__

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4PVPlacement.hh>
#include <G4VSensitiveDetector.hh>
#include <G4VSolid.hh>
#include <RAT/DB.hh>
#include <RAT/Factory.hh>
#include <RAT/GeoFactory.hh>
#include <RAT/GeoFiberSensitiveDetector.hh>
#include <string>
#include <vector>

namespace RAT {

struct GeoNestedTubeConstructionParams {
  GeoNestedTubeConstructionParams() { invisible = false; };

  bool invisible;

  double outer_r;
  double inner_r;
  double core_r;
  double Dz;  // half length

  G4Material *outer;
  G4Material *inner;
  G4Material *core;

  // G4OpticalSurface *outer_inner;
  G4OpticalSurface *inner_core;
};

class GeoNestedTubeConstruction {
 public:
  GeoNestedTubeConstruction(DBLinkPtr table, DBLinkPtr postable, G4LogicalVolume *mother, int ID);
  virtual ~GeoNestedTubeConstruction() {}

  virtual G4LogicalVolume *BuildVolume(const std::string &prefix, int ID, DBLinkPtr table);
  virtual G4VSolid *BuildSolid(const std::string &prefix);
  virtual G4PVPlacement *PlaceNestedTube(G4RotationMatrix *tuberot, G4ThreeVector tubepos, const std::string &name,
                                         G4LogicalVolume *logi_tube, G4VPhysicalVolume *mother_phys, bool booleanSolid,
                                         int copyNo);

 protected:
  // physical volumes
  G4PVPlacement *inner_phys;
  G4PVPlacement *core_phys;

  G4LogicalVolume *log_tube;
  GeoNestedTubeConstructionParams fParams;

  DBLinkPtr myTable;
};

}  // namespace RAT

#endif
