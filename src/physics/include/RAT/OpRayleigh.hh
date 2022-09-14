/*
 * Copyright (c) 2020 by the Snoplus collaboration
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Snoplus collaboration. The name of the
 * Snoplus collaboration may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
///////////////////////////////////////////////////////////////////////////////
//
// Simulates optical Rayleigh scattering. This class simulates
// Rayleigh scattering. It should be used in preference to the Geant4
// G4OpRayleigh class as it will calculate mean free paths for all
// materials not just water.
//
// Author: P G Jones <p.g.jones@qmul.ac.uk>
//
// REVISION HISTORY:
//     2014-01-08 : P G Jones - New file.
//     12-05-2015 : I Coulter - Remove use of the "SNOMAN style" RS_SCALE_FACTOR
//                              This already exists in SNOMANOpRayleigh, isn't
//                              used in general use and causes confusion.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __RAT_OpRayleigh_hh__
#define __RAT_OpRayleigh_hh__

#include <G4OpticalPhoton.hh>
#include <G4VDiscreteProcess.hh>

class G4PhysicsTable;
class G4MaterialPropertiesTable;

namespace RAT {

class OpRayleigh : public G4VDiscreteProcess {
 public:
  // Construct the class
  //
  // processName: optional, defaults to OpRayleigh
  // type: optional, defaults to fOptical
  OpRayleigh(const G4String &processName = "OpRayleigh", G4ProcessType type = fOptical);

  // Deletes the physics table
  ~OpRayleigh();

  // Returns true if this code is applicable to the particle type
  //
  // particleType: definition of the particle type
  // returns true if the particle type is an optical photon
  G4bool IsApplicable(const G4ParticleDefinition &particleType) {
    return (&particleType == G4OpticalPhoton::OpticalPhoton());
  }

  // Returns the mean free path for the track in mm.
  //
  // previousStepSize and condition are unused
  G4double GetMeanFreePath(const G4Track &track, G4double previousStepSize, G4ForceCondition *condition);

  // Invoke scattering to the track
  //
  // returns a particle change description
  G4VParticleChange *PostStepDoIt(const G4Track &track, const G4Step &step);

 private:
  // Builds the physics table, i.e. a table of mean free paths
  void BuildThePhysicsTable();

  G4PhysicsTable *fPhysicsTable;  // The physics table used
};

}  // namespace RAT

#endif
