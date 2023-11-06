#ifndef GDMLMessenger_hh
#define GDMLMessenger_hh

#include "G4UImessenger.hh"
#include "G4VPhysicalVolume.hh"
#include "globals.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithoutParameter;
class G4UIcmdWithABool;

namespace RAT {
class GDMLParser;

class GDMLMessenger : public G4UImessenger {
 public:
  GDMLMessenger(GDMLParser*);
  ~GDMLMessenger();

  void SetNewValue(G4UIcommand*, G4String);

 private:
  GDMLParser* myParser = nullptr;
  G4LogicalVolume* topvol = nullptr;

  G4UIdirectory* persistencyDir = nullptr;
  G4UIdirectory* gdmlDir = nullptr;
  G4UIcmdWithAString* ReaderCmd = nullptr;
  G4UIcmdWithAString* WriterCmd = nullptr;
  G4UIcmdWithAString* TopVolCmd = nullptr;
  G4UIcmdWithoutParameter* ClearCmd = nullptr;
  G4UIcmdWithABool* RegionCmd = nullptr;
  G4UIcmdWithABool* EcutsCmd = nullptr;
  G4UIcmdWithABool* SDCmd = nullptr;
  G4UIcmdWithABool* StripCmd = nullptr;
  G4UIcmdWithABool* AppendCmd = nullptr;

  G4bool pFlag = true;  // Append pointers to names flag
};
}  // namespace RAT
#endif
