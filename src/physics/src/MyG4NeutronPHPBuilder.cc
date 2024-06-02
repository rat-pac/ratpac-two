//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
#include "RAT/MyG4NeutronPHPBuilder.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4ProcessManager.hh"
#include "G4SystemOfUnits.hh"
#include "RAT/GdNeutronHPCapture.hh"

MyG4NeutronPHPBuilder::MyG4NeutronPHPBuilder() {
  theHPElastic = 0;
  theHPElasticData = 0;

  theHPFission = 0;
  theHPFissionData = 0;

  theHPCapture = 0;
  theHPCaptureData = 0;

  theHPInelastic = 0;
  theHPInelasticData = 0;

  theMin = 0;
  theIMin = theMin;
  theMax = 20 * MeV;
  theIMax = theMax;
}

void MyG4NeutronPHPBuilder::Build(G4HadronElasticProcess* aP) {
  if (theHPElastic == 0) theHPElastic = new G4ParticleHPElastic;
  theHPElastic->SetMinEnergy(theMin);
  theHPElastic->SetMaxEnergy(theMax);
  if (theHPElasticData == 0) theHPElasticData = new G4ParticleHPElasticData;
  aP->AddDataSet(theHPElasticData);
  aP->RegisterMe(theHPElastic);
}

void MyG4NeutronPHPBuilder::Build(G4NeutronFissionProcess* aP) {
  if (theHPFission == 0) theHPFission = new G4ParticleHPFission;
  theHPFission->SetMinEnergy(theMin);
  theHPFission->SetMaxEnergy(theMax);
  if (theHPFissionData == 0) theHPFissionData = new G4ParticleHPFissionData;
  aP->AddDataSet(theHPFissionData);
  aP->RegisterMe(theHPFission);
}

void MyG4NeutronPHPBuilder::Build(G4NeutronCaptureProcess* aP) {
  extern G4int MODEL;
  G4ProcessManager* pmanager = G4Neutron::Neutron()->GetProcessManager();
  if (MODEL == 4) {
    if (MyHPCapture == 0) GdNeutronHPCapture* MyHPCapture;
    MyHPCapture = new GdNeutronHPCapture;
    MyHPCapture->SetMinEnergy(theMin);
    MyHPCapture->SetMaxEnergy(theMax);
    //aP->RegisterMe(MyHPCapture);
    if (theHPCaptureData == 0) theHPCaptureData = new G4ParticleHPCaptureData;
    //pmanager->AddDiscreteProcess(aP);
    aP->AddDataSet(theHPCaptureData);
    aP->RegisterMe(MyHPCapture);
  } else {
    if (theHPCapture == 0) theHPCapture = new G4ParticleHPCapture;
    theHPCapture->SetMinEnergy(theMin);
    theHPCapture->SetMaxEnergy(theMax);
    if (theHPCaptureData == 0) theHPCaptureData = new G4ParticleHPCaptureData;
    aP->AddDataSet(theHPCaptureData);
    aP->RegisterMe(theHPCapture);
  }
}

void MyG4NeutronPHPBuilder::Build(G4HadronInelasticProcess* aP) {
  if (theHPInelastic == 0) theHPInelastic = new G4ParticleHPInelastic(G4Neutron::Neutron(), "NeutronHPInelastic");
  theHPInelastic->SetMinEnergy(theIMin);
  theHPInelastic->SetMaxEnergy(theIMax);
  if (theHPInelasticData == 0) theHPInelasticData = new G4ParticleHPInelasticData(G4Neutron::Neutron());
  aP->AddDataSet(theHPInelasticData);
  aP->RegisterMe(theHPInelastic);
}
