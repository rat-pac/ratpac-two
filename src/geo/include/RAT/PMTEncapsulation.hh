#ifndef __RAT_PMTEncapsulation__
#define __RAT_PMTEncapsulation__

#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4VSolid.hh>
#include <RAT/DB.hh>
#include <string>

namespace RAT {

class PMTEncapsulation {
 public:
  static PMTEncapsulation *NewConstruction(DBLinkPtr encaptable, DBLinkPtr pmttable, G4LogicalVolume *mother);

  PMTEncapsulation(std::string _name) : name(_name) {}

  virtual ~PMTEncapsulation() {}

  virtual G4VSolid *BuildSolid(const std::string &prefix) = 0;

  virtual G4LogicalVolume *BuildVolume(const std::string &prefix) = 0;

  virtual G4PVPlacement *PlaceEncap(G4RotationMatrix *pmtrot, G4ThreeVector pmtpos, const std::string &name,
                                    G4LogicalVolume *logi_pmt, G4VPhysicalVolume *mother_phys, bool booleanSolid,
                                    int copyNo) = 0;

 protected:
  std::string name;
};

}  // namespace RAT

#endif
