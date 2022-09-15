// RAT:: IBDgenMessenger
#ifndef RAT_IBDgenMessenger_hh
#define RAT_IBDgenMessenger_hh

#include "G4String.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UImessenger.hh"

// Forward declarations
class G4UIcommand;
class G4UIcmdWithADouble;

namespace RAT {

// Forward declarations within RAT namespace
class IBDgen;

class IBDgenMessenger : public G4UImessenger {
 public:
  IBDgenMessenger(IBDgen *);
  ~IBDgenMessenger();

  void SetNewValue(G4UIcommand *command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand *command);

 private:
  IBDgen *ibdgen;

  G4UIcmdWithAString *SpectrumUseCmd;
  G4UIcmdWithABool *NeutronUseCmd;
  G4UIcmdWithABool *PositronUseCmd;
};

}  // namespace RAT

#endif  // RAT_IBDgenMessenger_hh
