#include <G4RunManager.hh>
#include <RAT/GLG4VisManager.hh>
#include <RAT/Gsim.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/ProcBlock.hh>
#include <RAT/RunManager.hh>

namespace RAT {

RunManager::RunManager() {
  mainBlock = nullptr;
  Init();
}

RunManager::RunManager(ProcBlock *theMainBlock) {
  mainBlock = theMainBlock;
  Init();
}

void RunManager::Init() {
  theRunManager = new G4RunManager;  // Manages GEANT4 simulation process

  // Particle transport and interactions.  Note that this has to be
  // created outside of Gsim, since the physics list must be
  // initialized before the user tracking action.
  theRunManager->SetUserInitialization(new PhysicsList());

  // Create the  simulation manager.
  ratGsim = new Gsim(mainBlock);

  // Visualization, only if you choose to have it!
  theVisManager = new GLG4VisManager();
  theVisManager->Initialize();
}

RunManager::~RunManager() {
  delete ratGsim;
  delete theVisManager;
}

}  // namespace RAT
