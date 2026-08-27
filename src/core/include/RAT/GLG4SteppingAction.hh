// This file is part of the GenericLAND software library.
// $Id: GLG4SteppingAction.hh,v 1.1 2005/08/30 19:55:22 volsung Exp $
//
#ifndef __GLG4SteppingAction_H__
#define __GLG4SteppingAction_H__ 1

#include <string>
#include <vector>

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

  // "Direct light only" mode: keep an optical photon only while it is still on
  // the straight line it was emitted along.  A photon is killed as soon as it
  // is created by a process not listed in fDirectLightProcesses (reemission,
  // wavelength shifting, ...) or as soon as it scatters, wavelength shifts or
  // reflects.  Refraction into the next volume is kept, since a refracted
  // photon has still travelled straight from the emission point to the surface
  // it crossed.  Off by default; see /tracking/directLightOnly.
  static G4bool fDirectLightOnly;

  // Creator process names accepted by the direct light filter, matched as
  // substrings so that e.g. "Cerenkov" matches "G4CerenkovProcess".  Photons
  // with no creator process at all (primaries from a photon generator) are
  // always kept.  See /tracking/directLightProcesses.
  static std::vector<std::string> fDirectLightProcesses;

  // Optional extra cut for the direct light filter: kill a photon once the
  // cosine of the angle between its current direction and its emission
  // direction drops below this value.  Values <= -1 (the default) disable the
  // cut.  See /tracking/directLightMinCosine.
  static G4double fDirectLightMinCosine;

 private:
  // Apply the direct light filter to an optical photon.  Returns true if the
  // photon was killed.
  bool ApplyDirectLightFilter(const G4Step *aStep, G4Track *track);

  GLG4PrimaryGeneratorAction *myGenerator;
};

#endif
