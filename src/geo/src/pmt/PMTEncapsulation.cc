#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Region.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <RAT/HemisphereEncapsulation.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <RAT/PMTEncapsulation.hh>
#include <algorithm>

namespace RAT {

PMTEncapsulation *PMTEncapsulation::NewConstruction(DBLinkPtr encaptable, DBLinkPtr pmttable, G4LogicalVolume *mother) {
  std::string construction = encaptable->Get<std::string>("construction");
  if (construction == "hemisphere") {
    return new HemisphereEncapsulation(encaptable, pmttable, mother);
  } else {
    Log::Die("PMT encapsulation \'" + construction + "\' does not exist.");
  }
  return NULL;
}

}  // namespace RAT
