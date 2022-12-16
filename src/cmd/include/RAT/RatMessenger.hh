#ifndef RAT_RatMessenger_hh
#define RAT_RatMessenger_hh

#include <G4UIcmdWithABool.hh>
#include <G4UIcmdWithADouble.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIcmdWithAnInteger.hh>
#include <G4UIcommand.hh>
#include <G4UIparameter.hh>

#include "G4UImessenger.hh"

namespace RAT {
class RatMessenger : public G4UImessenger {
 public:
  RatMessenger();
  ~RatMessenger();

  void SetNewValue(G4UIcommand *command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand *command);

 protected:
  G4UIcmdWithADouble *fMaxWallTime;
};
}  // namespace RAT
#endif
