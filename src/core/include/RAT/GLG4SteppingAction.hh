// This file is part of the GenericLAND software library.
// $Id: GLG4SteppingAction.hh,v 1.1 2005/08/30 19:55:22 volsung Exp $
//
#ifndef __GLG4SteppingAction_H__
#define __GLG4SteppingAction_H__ 1

#include "G4ParticleChange.hh"
#include "G4UserSteppingAction.hh"
#include "globals.hh"

class GLG4PrimaryGeneratorAction;

class GLG4SteppingAction : public G4UserSteppingAction {
 public:
  static G4bool fUseGLG4;
  GLG4SteppingAction();
  void UserSteppingAction(const G4Step *aStep);

  // Kill a track if its global time exceeds this time.
  // Default is 0, or no time limit.
  static G4double max_global_time;

  // Kill every optical photon on its first step, so no optical propagation is
  // simulated. Set by the "profile" MC tuning option.
  static G4bool fKillOpticalPhotons;

 private:
  GLG4PrimaryGeneratorAction *myGenerator;
};

#endif
