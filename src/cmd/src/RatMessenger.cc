#include <RAT/Gsim.hh>
#include <RAT/Log.hh>
#include <RAT/RatMessenger.hh>
#include <string>

namespace RAT {

RatMessenger::RatMessenger() {
  // Build UI commands
  G4UIdirectory *ratdir = new G4UIdirectory("/rat/");
  ratdir->SetGuidance("General RAT commands");

  fMaxWallTime = new G4UIcmdWithADouble("/rat/maxWallTime", this);
  fMaxWallTime->SetGuidance("Max user time (seconds) to run before aborting the simulation (safely).");
  fMaxWallTime->SetParameterName("Time", false);
  fMaxWallTime->SetDefaultValue(0);
}

RatMessenger::~RatMessenger() { delete fMaxWallTime; }

G4String RatMessenger::GetCurrentValue(G4UIcommand *command) {
  if (command == fMaxWallTime) {
    return std::to_string(0);
  } else {
    Log::Die(dformat("RatMessenger::GetCurrentValue: Unknown command %s", command->GetCommandPath().data()));
  }
  return "";
}

void RatMessenger::SetNewValue(G4UIcommand *command, G4String newValue) {
  if (command == fMaxWallTime) {
    Gsim::SetMaxWallTime(G4UIcmdWithADouble::GetNewDoubleValue(newValue));
    // Gsim setter
  } else {
    Log::Die(dformat("RatMessenger::SetCurrentValue: Unknown command %s", command->GetCommandPath().data()));
  }
}

}  // namespace RAT
