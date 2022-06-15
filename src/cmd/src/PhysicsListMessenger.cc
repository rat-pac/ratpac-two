#include <string>
#include <G4UIcmdWithAString.hh>
#include <G4UIcmdWithAnInteger.hh>
#include <G4UIcmdWithABool.hh>
#include <RAT/PhysicsListMessenger.hh>
#include <RAT/Gsim.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/Log.hh>

namespace RAT {

PhysicsListMessenger::PhysicsListMessenger(PhysicsList* physicsList)
    : fPhysicsList(physicsList) {

  // Build UI commands
  G4UIdirectory* physicsdir = new G4UIdirectory("/rat/physics/");
  physicsdir->SetGuidance("PhysicsList commands");

  fSetOpWLSCmd = new G4UIcmdWithAString("/rat/physics/setOpWLS", this);
  fSetOpWLSCmd->SetParameterName("model", false);
  fSetOpWLSCmd->SetGuidance("Select a WLS model (g4|bnl)");
  fSetOpWLSCmd->SetDefaultValue("g4");

  fSetCerenkovMaxNumPhotonsPerStep = new G4UIcmdWithAnInteger("/rat/physics/setCerenkovMaxNumPhotonsPerStep", this);
  fSetCerenkovMaxNumPhotonsPerStep->SetParameterName("CerenkovMaxNumPhotonsPerStep", false);
  fSetCerenkovMaxNumPhotonsPerStep->SetGuidance("Indirectly controls the track step size");
  fSetCerenkovMaxNumPhotonsPerStep->SetDefaultValue(1);

  fEnableCerenkov = new G4UIcmdWithABool("/rat/physics/enableCerenkov", this);
  fEnableCerenkov->SetParameterName("EnableCerenkov", false);
  fEnableCerenkov->SetGuidance("Control Cerenkov production");
  fEnableCerenkov->SetDefaultValue(true);
}

PhysicsListMessenger::~PhysicsListMessenger() {
  delete fSetOpWLSCmd;
  delete fSetCerenkovMaxNumPhotonsPerStep;
  delete fEnableCerenkov;
}

G4String PhysicsListMessenger::GetCurrentValue(G4UIcommand* command) {
  if (command == fSetOpWLSCmd) {
    return G4String(fPhysicsList->GetOpWLSModelName().c_str());
  } else if (command == fSetCerenkovMaxNumPhotonsPerStep) {
    return std::to_string(fPhysicsList->GetCerenkovMaxNumPhotonsPerStep());
  } else if (command == fEnableCerenkov) {
    return fPhysicsList->GetCerenkovStatus() ? "True" : "False";
  } else {
    Log::Die(
      dformat("PhysicsListMessenger::GetCurrentValue: Unknown command %s",
      command->GetCommandPath().data()));
  }

  return "";
}

void PhysicsListMessenger::SetNewValue(G4UIcommand* command,
                                       G4String newValue) {
  info << "PhysicsListMessenger: Setting WLS model to " << newValue << newline;
  if (command == fSetOpWLSCmd) {
    fPhysicsList->SetOpWLSModel(std::string(newValue.data()));
  } else if (command == fSetCerenkovMaxNumPhotonsPerStep) {
    fPhysicsList->SetCerenkovMaxNumPhotonsPerStep( std::stoi(newValue) );
  } else if (command == fEnableCerenkov ) {
    fPhysicsList->EnableCerenkov( G4UIcmdWithABool::GetNewBoolValue(newValue) );
  } else {
    Log::Die(
      dformat("PhysicsListMessenger::SetCurrentValue: Unknown command %s",
              command->GetCommandPath().data()));
  }
}

}  // namespace RAT

