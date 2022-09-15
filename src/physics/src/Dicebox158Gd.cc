// fsutanto@umich.edu
// Apr 4, 2018

#include "RAT/Dicebox158Gd.hh"

#include <stdio.h>

#include <G4Event.hh>
#include <G4EventManager.hh>
#include <RAT/EventInfo.hh>
#include <RAT/Log.hh>
#include <RAT/Sampling.hh>
#include <RAT/TrackInfo.hh>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "G4Delete.hh"
#include "G4IonTable.hh"
#include "G4ParticleChange.hh"
#include "G4ParticleTable.hh"
#include "G4Timer.hh"
#include "G4TrackFastVector.hh"
#include "G4UnitsTable.hh"
#include "G4ios.hh"
#include "Randomize.hh"

// static
std::vector<Dicebox158Gd *> Dicebox158Gd::masterVector;

// constructor
Dicebox158Gd::Dicebox158Gd() {
  if (masterVector.size() == 0) {
    masterVector.push_back(this);
  }
}

// destructor
Dicebox158Gd::~Dicebox158Gd() {
  for (std::vector<Dicebox158Gd *>::iterator i = masterVector.begin(); i != masterVector.end(); i++) {
    if (*i == this) {
      masterVector.erase(i);
      break;
    }
  }
}

// PostPostStepDoIt
G4VParticleChange *Dicebox158Gd::PostStepDoIt(const G4Track &aTrack, const G4Step &aStep) {
  // intialize particle change
  aParticleChange.Initialize(aTrack);

  // Get the event and current track info
  G4Event *event = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
  RAT::EventInfo *eventInfo = dynamic_cast<RAT::EventInfo *>(event->GetUserInformation());
  RAT::TrackInfo *currentTrackInfo = dynamic_cast<RAT::TrackInfo *>(aTrack.GetUserInformation());

  if (eventInfo && eventInfo->StoreCapture158GdIDs) {
    // Only occurs on first step
    if (aTrack.GetCurrentStepNumber() == 1) {
      eventInfo->Capture158GdIDParentStep[aTrack.GetTrackID()].push_back(aTrack.GetParentID());
      eventInfo->Capture158GdIDParentStep[aTrack.GetTrackID()].push_back(currentTrackInfo->GetCreatorStep() - 1);
    }
  }

  // get info from the base particle
  G4ThreeVector x0 = aTrack.GetPosition();
  G4double t0 = aTrack.GetGlobalTime();

  // get info from ratdb
  RAT::DBLinkPtr model = RAT::DB::Get()->GetLink("DICEBOX158GD", "158gd");

  // set number of secondaries
  myCdf = model->GetDArray("mul_cdf");
  myMul = model->GetIArray("mul");
  G4double myRand = G4UniformRand();
  G4int indexNow = 0;
  while (1) {
    if (myCdf[indexNow] > myRand) break;
    indexNow++;
  }
  int numSecondaries = myMul[indexNow];
  aParticleChange.SetNumberOfSecondaries(numSecondaries);

  // get vector of energy for certain mutliplicity
  G4String tableErgName = "erg";
  tableErgName += std::to_string(numSecondaries);
  tableErgName += "_list";
  myErg = model->GetDArray(tableErgName);

  // get vector of particle type for certain mutliplicity
  G4String tableParName = "par";
  tableParName += std::to_string(numSecondaries);
  tableParName += "_list";
  myPar = model->GetIArray(tableParName);

  // Choose a set of data in the energy and particle type vector
  G4int chosenIndex = floor(G4UniformRand() * myPar.size() / numSecondaries);

  // for each new secondaries track
  for (int i = 0; i < numSecondaries; i++) {
    // propose the energy of new particle (or track)
    G4double erg = (myErg[numSecondaries * chosenIndex + i]) * CLHEP::MeV;

    // propose type of particle
    // 0=photon, 1,2=electrons (K,L shells)
    G4int type = (myPar[numSecondaries * chosenIndex + i]);

    // propose new random momentum
    G4double cost = 1. - 2. * G4UniformRand();
    G4double sint = sqrt(1. - cost * cost);

    G4double phi = 2. * CLHEP::pi * G4UniformRand();
    G4double sinp = sin(phi);
    G4double cosp = cos(phi);

    G4double px = sint * cosp;
    G4double py = sint * sinp;
    G4double pz = 0.0;
    if (G4UniformRand() > 0.5) {
      pz = cost;
    } else {
      pz = -cost;
    }

    G4ParticleMomentum p0(px, py, pz);

    // create new particle
    G4DynamicParticle *aParticle;
    if (type == 0) {
      aParticle = new G4DynamicParticle(G4Gamma::Gamma(), p0.unit(), erg);
    } else {
      aParticle = new G4DynamicParticle(G4Electron::Electron(), p0.unit(), erg);
    }

    // create new secondary track
    G4Track *aSecondaryTrack = new G4Track(aParticle, t0, x0);
    aSecondaryTrack->SetGoodForTrackingFlag();

    // set the track
    aSecondaryTrack->SetWeight(1.0);
    aSecondaryTrack->SetParentID(aTrack.GetTrackID());
    RAT::TrackInfo *trackInfo = new RAT::TrackInfo();
    trackInfo->SetCreatorStep(aTrack.GetCurrentStepNumber());
    trackInfo->SetCreatorProcess("nCapture");
    aSecondaryTrack->SetUserInformation(trackInfo);

    // Add the secondary to the ParticleChange object
    aParticleChange.SetSecondaryWeightByProcess(true);
    aParticleChange.AddSecondary(aSecondaryTrack);
    aSecondaryTrack->SetWeight(1.0);  // sometimes weight get overwritten
  }

  // aParticleChange.DumpInfo();
  return &aParticleChange;
}

// GenericPostPostStepDoIt
G4VParticleChange *Dicebox158Gd::GenericPostStepDoIt(const G4Step *pStep) {
  G4Track *track = pStep->GetTrack();
  std::vector<Dicebox158Gd *>::iterator it = masterVector.begin();

  return (*it)->PostStepDoIt(*track, *pStep);
}
