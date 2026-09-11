// RAT::PileupMessenger
// Provide user commands to allow the user to configure the multiplicity
// and per-vertex timing distributions of the PileupGen generator.

#include <G4String.hh>
#include <G4UIcommand.hh>
#include <G4UIdirectory.hh>
#include <G4UIparameter.hh>
#include <RAT/Log.hh>
#include <RAT/PileupGen.hh>
#include <RAT/PileupMessenger.hh>

namespace RAT {

PileupMessenger::PileupMessenger(PileupGen *gen) : fGen(gen) {
  // Commands live in a /generator/pileup/ directory
  G4UIdirectory *dir = new G4UIdirectory("/generator/pileup/");
  dir->SetGuidance("Control the parameters of the pileup generator");

  MultiplicityCmd = std::make_unique<G4UIcommand>("/generator/pileup/multiplicity", this);
  MultiplicityCmd->SetGuidance("Set the distribution for the number of vertices per event.");
  MultiplicityCmd->SetGuidance("Usage: /generator/pileup/multiplicity fixed:N | uniform:min:max | poisson:mean");
  MultiplicityCmd->SetParameter(new G4UIparameter("dist", 's', true));

  TimingCmd = std::make_unique<G4UIcommand>("/generator/pileup/timing", this);
  TimingCmd->SetGuidance("Set the distribution for vertex times (ns) relative to the event start time.");
  TimingCmd->SetGuidance(
      "fixed:dt places vertices dt apart; uniform:min:max draws each vertex's offset "
      "independently and uniformly from [min,max] and time-orders them; poisson:mean draws each "
      "vertex's offset independently from Exponential(mean) and time-orders them");
  TimingCmd->SetGuidance("Usage: /generator/pileup/timing fixed:dt | uniform:min:max | poisson:mean");
  TimingCmd->SetParameter(new G4UIparameter("dist", 's', true));
}

PileupMessenger::~PileupMessenger() {}

void PileupMessenger::SetNewValue(G4UIcommand *command, G4String newValues) {
  if (command == MultiplicityCmd.get()) {
    fGen->SetMultiplicityState(newValues);
  } else if (command == TimingCmd.get()) {
    fGen->SetVertexTimingState(newValues);
  } else {
    RAT::warn << "Error: PileupMessenger invalid command" << newline;
  }
}

G4String PileupMessenger::GetCurrentValue(G4UIcommand *command) {
  if (command == MultiplicityCmd.get()) {
    return fGen->GetMultiplicityState();
  } else if (command == TimingCmd.get()) {
    return fGen->GetVertexTimingState();
  }
  return G4String("invalid Pileup Messenger \"get\" command");
}

}  // namespace RAT
