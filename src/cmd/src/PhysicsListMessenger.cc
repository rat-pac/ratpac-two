#include <G4UIcmdWithABool.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIcmdWithAnInteger.hh>
#include <G4UIcommand.hh>
#include <G4UIparameter.hh>
#include <RAT/Gsim.hh>
#include <RAT/Log.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/PhysicsListMessenger.hh>
#include <string>

namespace RAT {

PhysicsListMessenger::PhysicsListMessenger(PhysicsList *physicsList) : fPhysicsList(physicsList) {
  // Build UI commands
  G4UIdirectory *physicsdir = new G4UIdirectory("/rat/physics/");
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

  fSetStepFunctionLightIons = new G4UIcommand("/rat/physics/setStepFunctionLightIons", this);
  fSetStepFunctionLightIons->SetGuidance("Set the energy loss step limitation parameters for light ions.");
  fSetStepFunctionLightIons->SetGuidance("  dRoverR:    max range variation per step");
  fSetStepFunctionLightIons->SetGuidance("  finalRange: range for final step");
  G4UIparameter *dRoverRLightIons = new G4UIparameter("dRoverRLightIons", 'd', false);
  fSetStepFunctionLightIons->SetParameter(dRoverRLightIons);
  G4UIparameter *finalRangeLightIons = new G4UIparameter("finalRangeLightIons", 'd', false);
  fSetStepFunctionLightIons->SetParameter(finalRangeLightIons);
  G4UIparameter *unitLightIons = new G4UIparameter("unit", 's', true);
  unitLightIons->SetDefaultValue("mm");
  fSetStepFunctionLightIons->SetParameter(unitLightIons);

  // fSetStepFunctionMuHad = new
  // G4UIcommand("/rat/physics/setStepFunctionLightIons", this);
  fSetStepFunctionMuHad = new G4UIcommand("/rat/physics/setStepFunctionMuHad", this);
  fSetStepFunctionMuHad->SetGuidance("Set the energy loss step limitation parameters for light ions.");
  fSetStepFunctionMuHad->SetGuidance("  dRoverR:    max range variation per step");
  fSetStepFunctionMuHad->SetGuidance("  finalRange: range for final step");
  G4UIparameter *dRoverRMuHad = new G4UIparameter("dRoverRMuHad", 'd', false);
  fSetStepFunctionMuHad->SetParameter(dRoverRMuHad);
  G4UIparameter *finalRangeMuHad = new G4UIparameter("finalRangeMuHad", 'd', false);
  fSetStepFunctionMuHad->SetParameter(finalRangeMuHad);
  G4UIparameter *unitMuHad = new G4UIparameter("unit", 's', true);
  unitMuHad->SetDefaultValue("mm");
  fSetStepFunctionMuHad->SetParameter(unitMuHad);
}

PhysicsListMessenger::~PhysicsListMessenger() {
  delete fSetOpWLSCmd;
  delete fSetCerenkovMaxNumPhotonsPerStep;
  delete fEnableCerenkov;
  delete fSetStepFunctionLightIons;
  delete fSetStepFunctionMuHad;
}

G4String PhysicsListMessenger::GetCurrentValue(G4UIcommand *command) {
  if (command == fSetOpWLSCmd) {
    return G4String(fPhysicsList->GetOpWLSModelName().c_str());
  } else if (command == fSetCerenkovMaxNumPhotonsPerStep) {
    return std::to_string(fPhysicsList->GetCerenkovMaxNumPhotonsPerStep());
  } else if (command == fEnableCerenkov) {
    return fPhysicsList->GetCerenkovStatus() ? "True" : "False";
  } else {
    Log::Die(dformat("PhysicsListMessenger::GetCurrentValue: Unknown command %s", command->GetCommandPath().data()));
  }

  return "";
}

void PhysicsListMessenger::SetNewValue(G4UIcommand *command, G4String newValue) {
  info << "PhysicsListMessenger: Setting WLS model to " << newValue << newline;
  if (command == fSetOpWLSCmd) {
    fPhysicsList->SetOpWLSModel(std::string(newValue.data()));
  } else if (command == fSetCerenkovMaxNumPhotonsPerStep) {
    fPhysicsList->SetCerenkovMaxNumPhotonsPerStep(std::stoi(newValue));
  } else if (command == fEnableCerenkov) {
    fPhysicsList->EnableCerenkov(G4UIcmdWithABool::GetNewBoolValue(newValue));
  } else if (command == fSetStepFunctionLightIons || command == fSetStepFunctionMuHad) {
    G4double v1, v2;
    G4String unt;
    std::istringstream is(newValue);
    is >> v1 >> v2 >> unt;
    v2 *= G4UIcommand::ValueOf(unt);
    if (command == fSetStepFunctionLightIons) {
      fPhysicsList->SetStepFunctionLightIons(v1, v2);
    } else if (command == fSetStepFunctionMuHad) {
      fPhysicsList->SetStepFunctionMuHad(v1, v2);
    }
  } else {
    Log::Die(dformat("PhysicsListMessenger::SetCurrentValue: Unknown command %s", command->GetCommandPath().data()));
  }
}

}  // namespace RAT
