#ifndef __RAT_VertexGen_CRY__
#define __RAT_VertexGen_CRY__

#include <TTimeStamp.h>

#include <G4Event.hh>
#include <G4ThreeVector.hh>
#include <RAT/GLG4VertexGen.hh>
#include <globals.hh>

#include "RAT/DB.hh"

class CRYGenerator;
namespace RAT {

class CRYGenMessenger;
// Generate inverse beta decay event
class VertexGen_CRY : public GLG4VertexGen {
 public:
  VertexGen_CRY(const char *arg_dbname = "cry");
  void GeneratePrimaryVertex(G4Event *, G4ThreeVector &, G4double);
  void SetState(G4String newValues);
  G4String GetState();

 private:
  CRYGenerator *generator;
  bool returnNeutrons;
  bool returnProtons;
  bool returnGammas;
  bool returnElectrons;
  bool returnMuons;
  bool returnPions;
  int nParticlesMin;
  int nParticlesMax;
  double altitude;
  double latitude;
  std::string date;
  int subboxLength;
  TTimeStamp startTime;
};

}  // namespace RAT

#endif
