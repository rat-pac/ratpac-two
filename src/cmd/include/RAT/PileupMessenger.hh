// RAT::PileupMessenger
// Provide user commands to allow the user to configure the multiplicity
// and per-vertex timing distributions of the PileupGen generator.

#ifndef RAT_PileupMessenger_hh
#define RAT_PileupMessenger_hh

#include <memory>

#include "G4String.hh"
#include "G4UImessenger.hh"

class G4UIcommand;

namespace RAT {

class PileupGen;

class PileupMessenger : public G4UImessenger {
 public:
  PileupMessenger(PileupGen *);
  ~PileupMessenger();

  void SetNewValue(G4UIcommand *command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand *command);

 private:
  PileupGen *fGen;

  std::unique_ptr<G4UIcommand> MultiplicityCmd;
  std::unique_ptr<G4UIcommand> TimingCmd;
};

}  // namespace RAT

#endif  // RAT_PileupMessenger_hh
