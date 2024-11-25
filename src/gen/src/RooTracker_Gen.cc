#include <TROOT.h>
#include <TTimeStamp.h>
#include <TTree.h>
#include <TVector3.h>

#include <G4Event.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4RunManager.hh>
#include <G4ThreeVector.hh>
#include <RAT/DS/MC.hh>
#include <RAT/DS/MCParticle.hh>
#include <RAT/DS/Root.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4PosGen.hh>
#include <RAT/GLG4StringUtil.hh>
#include <RAT/GLG4TimeGen.hh>
#include <RAT/Log.hh>
#include <RAT/PrimaryVertexInformation.hh>
#include <RAT/RooTracker_Gen.hh>
#include <stdexcept>
#include <string>
#include <vector>

namespace RAT {

void RooTracker_Gen::GenerateEvent(G4Event* event) {
  fTTree->GetEntry(fCurrentEvent);

  // Vertex position
  G4ThreeVector pos;
  if (fPosGen) {
    fPosGen->GeneratePosition(pos);
  } else {
    pos = G4ThreeVector(fRec->EvtVtx[0] * 1000, fRec->EvtVtx[1] * 1000, fRec->EvtVtx[2] * 1000);
  }

  // Vertex time
  double time;
  if (fTimeGen) {
    time = NextTime();
  } else {
    time = fRec->EvtVtx[3] * 1e9;
  }

  G4PrimaryVertex* vertex = new G4PrimaryVertex(pos, time);
  PrimaryVertexInformation* vertinfo = new PrimaryVertexInformation();

  // Loop over StdHep particles
  for (int i = 0; i < fRec->StdHepN; i++) {
    int status = fRec->StdHepStatus[i];

    if (status == 0 || status == 1) {
      G4PrimaryParticle* p = new G4PrimaryParticle(fRec->StdHepPdg[i], fRec->StdHepP4[i][0] * 1000,
                                                   fRec->StdHepP4[i][1] * 1000, fRec->StdHepP4[i][2] * 1000);

      status == 0 ? vertinfo->AddNewParentParticle(p) : vertex->SetPrimary(p);
    }
  }

  vertex->SetUserInformation(vertinfo);
  event->AddPrimaryVertex(vertex);

  fLastEventTime = time;
  fCurrentEvent++;

  if (fCurrentEvent >= fNumEvents || (fMaxEvent > 0 && fCurrentEvent >= fMaxEvent)) {
    G4RunManager::GetRunManager()->AbortRun(true);
  }
}

void RooTracker_Gen::ResetTime(double offset) {
  if (fCurrentEvent < fNumEvents) {
    if (fTimeGen) {
      double eventTime = fTimeGen->GenerateEventTime();
      nextTime = eventTime + offset;
    } else {
      fTTree->GetEntry(fCurrentEvent);
      double time = fRec->EvtVtx[3] * 1e9;
      nextTime = time - fLastEventTime + offset;
    }
  } else {
    nextTime = 1e9;
  }
}

void RooTracker_Gen::SetState(G4String state) {
  // Break the argument to the this generator into sub-std::strings
  // separated by ":".
  state = util_strip_default(state);
  std::vector<std::string> parts = util_split(state, ":");
  size_t nArgs = parts.size();

  std::string filename;
  if (nArgs >= 5) {
    int num_skip;
    std::istringstream(parts[4]) >> num_skip;
    fCurrentEvent = num_skip;
  }
  if (nArgs >= 4) {
    std::istringstream(parts[3]) >> fMaxEvent;
    if (fMaxEvent > 0) fMaxEvent += fCurrentEvent;
  }
  if (nArgs >= 3) {
    if (parts[2] == "default") {
      fTimeGen = 0;
    } else {
      fTimeGen = GlobalFactory<GLG4TimeGen>::New(parts[2]);
    }
  }
  if (nArgs >= 2) {
    if (parts[1] == "default") {
      fPosGen = 0;
    } else {
      fPosGen = GlobalFactory<GLG4PosGen>::New(parts[1]);
    }
  }
  if (nArgs >= 1) {
    filename = parts[0];
  } else {
    G4Exception(__FILE__, "Invalid Parameter", FatalException,
                ("rootracker generator syntax error: '" + state + "' does not have a filename").c_str());
  }

  fStateStr = state;
  fFile = TFile::Open(filename.c_str());
  gROOT->cd(0);
  TTree* t = (TTree*)fFile->Get("gRooTracker");
  fTTree = (TTree*)t->CloneTree();
  fTTree->SetDirectory(0);
  fFile->Close();

  fNumEvents = fTTree->GetEntries();

  if (!fNumEvents)
    G4Exception(__FILE__, "Invalid Parameter", FatalException, ("File '" + filename + "' is empty").c_str());

  fRec = new StdHepRecord(fTTree);
}

G4String RooTracker_Gen::GetState() const { return fStateStr; }

void RooTracker_Gen::SetTimeState(G4String state) {
  if (fTimeGen)
    fTimeGen->SetState(state);
  else
    warn << "RooTracker_Gen error: Cannot set time state, no time generator "
            "selected\n";
}

G4String RooTracker_Gen::GetTimeState() const {
  if (fTimeGen)
    return fTimeGen->GetState();
  else
    return G4String("RooTracker_Gen error: no time generator selected");
}

void RooTracker_Gen::SetPosState(G4String state) {
  if (fPosGen)
    fPosGen->SetState(state);
  else
    warn << "RooTracker_Gen error: Cannot set position state, no position "
            "generator selected\n";
}

G4String RooTracker_Gen::GetPosState() const {
  if (fPosGen)
    return fPosGen->GetState();
  else
    return G4String("RooTracker_Gen error: no pos generator selected");
}

}  // namespace RAT
