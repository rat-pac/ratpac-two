#include <RAT/WLSPCoverFactory.hh>
#include <G4SubtractionSolid.hh>
#include <G4Box.hh>
#include <RAT/Log.hh>
#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>


using namespace std;

namespace RAT {
//A simple geometry factory that creates a hollow box (or square ring) by subtracting one G4Box from another
G4VPhysicalVolume *WLSPCoverFactory::Construct(DBLinkPtr table)
{
  string volume_name        = table->GetIndex();
  const vector<double> &outsize = table->GetDArray("outer_size");
  G4VSolid* outbox = new G4Box(volume_name, outsize[0] * CLHEP::mm, outsize[1] * CLHEP::mm, outsize[2] * CLHEP::mm);
  
  const vector<double> &insize = table->GetDArray("inner_size");
  G4VSolid* inbox = new G4Box(volume_name, insize[0] * CLHEP::mm, insize[1] * CLHEP::mm, insize[2] * CLHEP::mm);
  
  outbox = new G4SubtractionSolid(volume_name, outbox, inbox);
  return GeoSolidArrayFactoryBase::Construct(outbox, table);
  //return box;

}
} // namespace RAT
