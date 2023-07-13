#include <G4String.hh>
#include <G4UIcmdWithABool.hh>
#include <G4UIcmdWithADouble.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIcommand.hh>
#include <G4UIdirectory.hh>
#include <RAT/IBDgen.hh>
#include <RAT/IBDgenMessenger.hh>
#include <RAT/Log.hh>

namespace RAT {
IBDgenMessenger::IBDgenMessenger(IBDgen *re) : ibdgen(re) {
  // Commands will be called in the mac with /generator/ibd/
  G4UIdirectory *dir = new G4UIdirectory("/generator/ibd/");
  dir->SetGuidance("Control the IBD neutrino spectrum of the IBD generator");

  // Choose the positron spectrum
  SpectrumUseCmd = new G4UIcmdWithAString("/generator/ibd/spectrum", this);
  SpectrumUseCmd->SetGuidance("Set the spectrum (index of the IBD ratdb table)");
  SpectrumUseCmd->SetParameterName("Spectrum", false);
  SpectrumUseCmd->SetDefaultValue("default");

  // Toggle neutrons, by default we want neutrons
  NeutronUseCmd = new G4UIcmdWithABool("/generator/ibd/neutron", this);
  NeutronUseCmd->SetGuidance("Toggle neutron generation");
  NeutronUseCmd->SetParameterName("n_toggle", true, false);
  NeutronUseCmd->SetDefaultValue(true);

  // Toggle positrons, by default we want positrons
  PositronUseCmd = new G4UIcmdWithABool("/generator/ibd/positron", this);
  PositronUseCmd->SetGuidance("Toggle positron generation");
  PositronUseCmd->SetParameterName("e_toggle", true, false);
  PositronUseCmd->SetDefaultValue(true);
}

IBDgenMessenger::~IBDgenMessenger() { delete SpectrumUseCmd; }

void IBDgenMessenger::SetNewValue(G4UIcommand *command, G4String newValue) {
  if (command == SpectrumUseCmd) {
    ibdgen->SetSpectrumIndex(newValue);
  } else if (command == NeutronUseCmd) {
    ibdgen->SetNeutronState(NeutronUseCmd->GetNewBoolValue(newValue));
  } else if (command == PositronUseCmd) {
    ibdgen->SetPositronState(PositronUseCmd->GetNewBoolValue(newValue));
  } else {
    RAT::warn << "Error: Invalid IBDgenMessenger \"set\" command" << newline;
  }
}

G4String IBDgenMessenger::GetCurrentValue(G4UIcommand *command) {
  if (command == SpectrumUseCmd) {
    return ibdgen->GetSpectrumIndex();
  }
  // Get here, you return an error.
  return G4String("Error: Invalid IBDgenMessenger \"get\" command");
}

}  // namespace RAT
