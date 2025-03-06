#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Region.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <RAT/CubicPMTConstruction.hh>
#include <RAT/CylindricalPMTConstruction.hh>
#include <RAT/GLG4PMTOpticalModel.hh>
#include <RAT/LAPPDConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <RAT/PMTConstruction.hh>
#include <RAT/RevolutionPMTConstruction.hh>
#include <RAT/ToroidalPMTConstruction.hh>
#include <algorithm>

namespace RAT {

PMTConstruction *PMTConstruction::NewConstruction(DBLinkPtr table, G4LogicalVolume *mother) {
  std::string construction = table->Get<std::string>("construction");
  if (construction == "toroidal") {
    return new ToroidalPMTConstruction(table, mother);
  } else if (construction == "revolution") {
    return new RevolutionPMTConstruction(table, mother);
  } else if (construction == "cylindrical") {
    return new CylindricalPMTConstruction(table, mother);
  } else if (construction == "cubic") {
    return new CubicPMTConstruction(table, mother);
  } else if (construction == "lappd") {
    return new LAPPDConstruction(table, mother);
  } else {
    Log::Die("PMT construction \'" + construction + "\' does not exist.");
  }
  return NULL;
}

}  // namespace RAT
