// fsutanto@umich.edu
// Apr 15,2018
// the class is added to kill secondary tracks
// that are produced by neutron capture on 158Gd

#include "RAT/StackingAction.hh"

#include <RAT/Log.hh>

#include "G4Electron.hh"
#include "G4Gamma.hh"
#include "G4NeutrinoE.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

// constructor
StackingAction::StackingAction() {}

// destructor
StackingAction::~StackingAction() {}

// Classify new tracks
G4ClassificationOfNewTrack StackingAction::ClassifyNewTrack(const G4Track *track) {
  G4ParticleDefinition *par = track->GetDefinition();
  G4String nameParticle = par->GetParticleName();

#define debug_dicebox
#undef debug_dicebox
#ifdef debug_dicebox

  if (nameParticle != "opticalphoton" && track->GetParentID() == 1) {
    RAT::debug << "*******************************" << newline;
    RAT::debug << "      Classify new track       " << newline;
    RAT::debug << "*******************************" << newline;

    RAT::debug << "(name, parentID,trackID,stepID,status) : (" << par->GetParticleName() << "," << track->GetParentID()
               << "," << track->GetTrackID() << "," << track->GetCurrentStepNumber() << "," << track->GetTrackStatus()
               << ")" << newline;
  }

#endif

  // In SteppingAction.cc, if a step is neutron capture on 157Gd,
  // we postpone all of its secondaries except the 158Gd.
  // These secondaries are then send here (StackingAction.cc)
  // to be removed.
  // GetTrackStatus()==5 is the fPostponeToNextEvent

  if (track->GetTrackStatus() == 5 && (nameParticle == "gamma" || nameParticle == "e-")) {
    return fKill;
  } else {
    return fUrgent;
  }
}
