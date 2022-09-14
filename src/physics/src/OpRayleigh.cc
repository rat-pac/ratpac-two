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

#include <G4OpProcessSubType.hh>
#include <RAT/OpRayleigh.hh>
#include <RAT/TrackInfo.hh>
#include <Randomize.hh>

namespace RAT {

OpRayleigh::OpRayleigh(const G4String &processName, G4ProcessType type) : G4VDiscreteProcess(processName, type) {
  SetProcessSubType(fOpRayleigh);

  fPhysicsTable = nullptr;
  BuildThePhysicsTable();
}

OpRayleigh::~OpRayleigh() {
  if (fPhysicsTable != nullptr) {
    fPhysicsTable->clearAndDestroy();
    delete fPhysicsTable;
  }
}

G4VParticleChange *OpRayleigh::PostStepDoIt(const G4Track &track, const G4Step &step) {
  aParticleChange.Initialize(track);
  const G4DynamicParticle *aParticle = track.GetDynamicParticle();

  double psi = 0.0;
  do {
    psi = CLHEP::twopi * G4UniformRand() / 2.0;
  } while (G4UniformRand() > pow(sin(psi), 3));
  const double phi = G4UniformRand() * CLHEP::twopi;

  const G4ThreeVector oldMomentum = aParticle->GetMomentumDirection().unit();
  const G4ThreeVector oldPolarisation = aParticle->GetPolarization().unit();
  const G4ThreeVector e2 = oldMomentum.cross(oldPolarisation);

  // oldPolarisation is Z, oldMomentum is x and e2 y
  G4ThreeVector newMomentum = cos(psi) * oldPolarisation + sin(psi) * cos(phi) * oldMomentum + sin(psi) * sin(phi) * e2;
  newMomentum.unit();

  G4ThreeVector newPolarisation = oldPolarisation - cos(psi) * newMomentum;
  newPolarisation.unit();

  aParticleChange.ProposePolarization(newPolarisation);
  aParticleChange.ProposeMomentumDirection(newMomentum);

  RAT::TrackInfo *trackHistory = dynamic_cast<RAT::TrackInfo *>(track.GetUserInformation());
  if (trackHistory) {
    // trackHistory->AddToHistory(RAT::DS::MCPE::hRayleighScatt); //FIXME
    // rat-pac does not have
  }

  return G4VDiscreteProcess::PostStepDoIt(track, step);
}

G4double OpRayleigh::GetMeanFreePath(const G4Track &track, G4double, G4ForceCondition *) {
  const G4DynamicParticle *particle = track.GetDynamicParticle();
  const G4double photonMomentum = particle->GetTotalMomentum();
  const G4Material *material = track.GetMaterial();
  G4PhysicsFreeVector *rayleigh = static_cast<G4PhysicsFreeVector *>((*fPhysicsTable)(material->GetIndex()));

  G4double rsLength = DBL_MAX;
  if (rayleigh != nullptr) rsLength = rayleigh->Value(photonMomentum);
  return rsLength;
}

void OpRayleigh::BuildThePhysicsTable() {
  if (fPhysicsTable) return;
  const G4MaterialTable *theMaterialTable = G4Material::GetMaterialTable();
  const G4int numOfMaterials = G4Material::GetNumberOfMaterials();
  fPhysicsTable = new G4PhysicsTable(numOfMaterials);

  for (G4int iMaterial = 0; iMaterial < numOfMaterials; iMaterial++) {
    G4Material *material = (*theMaterialTable)[iMaterial];
    G4MaterialPropertiesTable *materialProperties = material->GetMaterialPropertiesTable();
    G4PhysicsFreeVector *rayleigh = nullptr;
    if (materialProperties != nullptr) {
      rayleigh = materialProperties->GetProperty("RSLENGTH");
    }
    fPhysicsTable->insertAt(iMaterial, rayleigh);
  }
}
}  // namespace RAT
