// This file is part of the GenericLAND software library.
// $Id: GLG4SteppingAction.cc,v 1.1 2005/08/30 19:55:23 volsung Exp $
//
//
//  GenericLAND Simulation
//
//  Concrete implementation of G4UserSteppingAction
//
//  Current uses:
//    * Measure inter-step CPU time, broken down by process and particle type
//
//  Anticipated uses:
//    * Find PMT _fast_ when entering outer buffer
//
//  Author: Glenn Horton-Smith, April 7, 2000

#include "RAT/GLG4SteppingAction.hh"

#include <RAT/TrackInfo.hh>
#include <RAT/Log.hh>

#include "CLHEP/Units/PhysicalConstants.h"
#include "G4OpticalPhoton.hh"
#include "G4ParticleChange.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SteppingManager.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4ios.hh"
#include "RAT/Dicebox158Gd.hh"
#include "RAT/GLG4PrimaryGeneratorAction.hh"
#include "RAT/GLG4Scint.hh"
#include "globals.hh"

GLG4SteppingAction::GLG4SteppingAction() {
  myGenerator = GLG4PrimaryGeneratorAction::GetTheGLG4PrimaryGeneratorAction();
  if (myGenerator == 0) {
    G4Exception(__FILE__, "No Primary Generator", FatalException,
                "GLG4SteppingAction:: no GLG4PrimaryGeneratorAction instance.");
  }
}

#ifdef G4DEBUG
#include "G4Timer.hh"
#include "map"

struct GLG4SteppingAction_time_s {
  G4double sumtime;
  G4int stepcount;
  GLG4SteppingAction_time_s() {
    sumtime = 0.0;
    stepcount = 0;
  }
  void sum(G4double time) {
    sumtime += time;
    stepcount++;
  }
};

typedef std::map<G4String, GLG4SteppingAction_time_s> GLG4SteppingAction_time_map;
typedef std::map<G4String, GLG4SteppingAction_time_s>::iterator GLG4SteppingAction_time_map_iterator;

GLG4SteppingAction_time_map GLG4SteppingAction_times;
G4double GLG4SteppingAction_internal_time = 0.0;

int GLG4SteppingAction_dump_times(void) {
  for (GLG4SteppingAction_time_map_iterator i = GLG4SteppingAction_times.begin(); i != GLG4SteppingAction_times.end();
       i++) {
    RAT::info << i->first << ' ' << i->second.sumtime << ' ' << i->second.stepcount << newline;
  }
  return 1;
}

#include "G4Color.hh"
#include "G4VisAttributes.hh"

const int nrowIlluminationMap = 600;
const int ncolIlluminationMap = 600;
G4double widthIlluminationMap = 18.0 * m;
G4double IlluminationMap[6][nrowIlluminationMap][ncolIlluminationMap][3];

static void updateIlluminationMap(int imap, G4double srow, G4double scol, const G4Color *cp) {
  G4double *mp = IlluminationMap[imap][(int)(srow * nrowIlluminationMap)][(int)(scol * ncolIlluminationMap)];
  mp[0] += cp->GetRed();
  mp[1] += cp->GetGreen();
  mp[2] += cp->GetBlue();
}

int GLG4SteppingAction_dump_IlluminationMap(void) {
  G4double maxval = 0.0, meanval = 0.0;
  G4int nsum = 0;
  {
    for (int kmap = 0; kmap < 6; kmap++)
      for (int irow = 0; irow < nrowIlluminationMap; irow++)
        for (int jcol = 0; jcol < ncolIlluminationMap; jcol++)
          for (int kcol = 0; kcol < 3; kcol++) {
            G4double v = IlluminationMap[kmap][irow][jcol][kcol];
            if (v > 0.0) {
              if (v > maxval) maxval = v;
              meanval += v;
              nsum++;
            }
          }
  }
  if (maxval == 0.0) {
    RAT::info << "Empty Illumination Map" << newline;
    return 0;
  }
  meanval /= nsum;
  {
    for (int kmap = 0; kmap < 6; kmap++) {
      static char filename[] = "map#.ppm";
      filename[3] = kmap + '0';
      std::ofstream of(filename);
      of << "P6\n# Illumination map " << kmap << newline << nrowIlluminationMap << ' ' << nrowIlluminationMap << " 255" << newline;
      for (int irow = 0; irow < nrowIlluminationMap; irow++) {
        for (int jcol = 0; jcol < ncolIlluminationMap; jcol++)
          for (int kcol = 0; kcol < 3; kcol++) {
            G4double v = IlluminationMap[kmap][irow][jcol][kcol];
            unsigned char byte;
            if (v < meanval)
              byte = (unsigned char)(128 * v / meanval);
            else
              byte = (unsigned char)(128.0 + 127.99 * (v - meanval) / (maxval - meanval));
            of.put(byte);
          }
      }
      of.close();
    }
  }
  return 1;
}

G4double GLG4SteppingAction_totEdep = 0.0;
#endif /* G4DEBUG */

G4int GLG4SteppingAction_MaxStepNumber = 100000;
G4double GLG4SteppingAction::max_global_time = 0.0;
G4bool GLG4SteppingAction::fUseGLG4 = true;

void GLG4SteppingAction::UserSteppingAction(const G4Step *aStep) {
  G4Track *track = aStep->GetTrack();
  static G4int num_zero_steps_in_a_row = 0;

  // check for too many zero steps in a row
  if (aStep->GetStepLength() <= 0.0 && track->GetCurrentStepNumber() > 1) {
    ++num_zero_steps_in_a_row;
    if (num_zero_steps_in_a_row >= 4) {
      const G4VPhysicalVolume *pv = track->GetVolume();
      const G4VProcess *lastproc = track->GetStep()->GetPostStepPoint()->GetProcessDefinedStep();
      G4cerr << "GLG4SteppingAction: Too many zero steps for this track, "
                "terminating!"
             << " type=" << track->GetDefinition()->GetParticleName()
             << "\n volume=" << (pv != 0 ? pv->GetName() : G4String("NULL"))
             << " last_process=" << (lastproc != 0 ? lastproc->GetProcessName() : G4String("NULL"))
             << "\n position=" << track->GetPosition() << " momentum=" << track->GetMomentum() << newline;
      track->SetTrackStatus(fStopAndKill);
      num_zero_steps_in_a_row = 0;
    }
  } else
    num_zero_steps_in_a_row = 0;

  // check for very high number of steps
  if (track->GetCurrentStepNumber() % GLG4SteppingAction_MaxStepNumber == 0) {
    RAT::warn << "warning:  step_no=" << track->GetCurrentStepNumber() << "  %  " << GLG4SteppingAction_MaxStepNumber
           << " == 0 for particle: " << track->GetDefinition()->GetParticleName() << newline;
  }

  if (track->GetCurrentStepNumber() > GLG4SteppingAction_MaxStepNumber &&
      !(track->GetDefinition()->GetParticleName() == "mu-" || track->GetDefinition()->GetParticleName() == "mu+")) {
    const G4VPhysicalVolume *pv = track->GetVolume();
    const G4VProcess *lastproc = track->GetStep()->GetPostStepPoint()->GetProcessDefinedStep();
    G4cerr << "GLG4SteppingAction: Too many steps for this track, terminating!" << newline
           << " step_no=" << track->GetCurrentStepNumber() << " type=" << track->GetDefinition()->GetParticleName()
           << "\n volume=" << (pv != 0 ? pv->GetName() : G4String("NULL"))
           << " last_process=" << (lastproc != 0 ? lastproc->GetProcessName() : G4String("NULL"))
           << "\n position=" << track->GetPosition() << " momentum=" << track->GetMomentum() << newline;
    track->SetTrackStatus(fStopAndKill);
  }

  if (max_global_time > 0.0) {
    double gtime = track->GetGlobalTime();
    if (gtime > max_global_time) {
      /*
        KJP 8/17/2011 Commented out annoying error message that fills logs
      const G4VPhysicalVolume* pv= track->GetVolume();
      const G4VProcess* lastproc= track->GetStep()->GetPostStepPoint()
        ->GetProcessDefinedStep();


      G4cerr << "GLG4SteppingAction: Track time = " << gtime
             << " exceeds max_global_time = " << max_global_time
             << " step_no=" << track->GetCurrentStepNumber()
             << " type=" << track->GetDefinition()->GetParticleName()
             << "\n volume=" << (pv!=0 ? pv->GetName() : G4String("NULL"))
             << " last_process="
             << (lastproc!=0 ? lastproc->GetProcessName() : G4String("NULL"))
             << "\n position=" << track->GetPosition()
             << " momentum=" << track->GetMomentum()
             << newline;
      */

      track->SetTrackStatus(fStopAndKill);
    }
  }

  //===========================================================================//
  // DICEBOX 158Gd //
  //===========================================================================//

#define debug_dicebox158Gd
#undef debug_dicebox158Gd

  // check if the process is neutron capture on 157Gd
  bool nCap157Gd = false;
  const G4VProcess *myproc = aStep->GetPostStepPoint()->GetProcessDefinedStep();
  G4String nameProcess = "Unknown";
  if (myproc != 0) nameProcess = myproc->GetProcessName();

  G4TrackVector *fSecondary = fpSteppingManager->GetfSecondary();
  G4int numSecondaries = fpSteppingManager->GetfN2ndariesAtRestDoIt() +
                         fpSteppingManager->GetfN2ndariesAlongStepDoIt() +
                         fpSteppingManager->GetfN2ndariesPostStepDoIt();

  if (nameProcess == "nCapture") {
#ifdef debug_dicebox158Gd
    RAT::debug << newline << "=======================BEFORE DICEBOX======================" << newline;
#endif

    if (numSecondaries > 0) {
      for (size_t lp1 = (*fSecondary).size() - numSecondaries; lp1 < (*fSecondary).size(); lp1++) {
        G4ParticleDefinition *par = (*fSecondary)[lp1]->GetDefinition();
        G4String nameParticle = par->GetParticleName();

#ifdef debug_dicebox158Gd
        G4cout << "(i,name,pos,momentum,erg,time) : (" << lp1 << "," << par->GetParticleName() << ","
               << (*fSecondary)[lp1]->GetPosition() << "," << (*fSecondary)[lp1]->GetMomentum() << ","
               << (*fSecondary)[lp1]->GetKineticEnergy() << "," << (*fSecondary)[lp1]->GetGlobalTime() << ")" << newline;

        G4cout << "(parentID,trackID,stepID,status) : (" << (*fSecondary)[lp1]->GetParentID() << ","
               << (*fSecondary)[lp1]->GetTrackID() << "," << (*fSecondary)[lp1]->GetCurrentStepNumber() << ","
               << (*fSecondary)[lp1]->GetTrackStatus() << ")" << newline;
#endif
        if (nameParticle == "Gd158") {
          nCap157Gd = true;
        }
      }
    }
  }

  // If the step is neutron capture on 157Gd, then postpone all of its
  // secondaries except the Gd158. Then we get the new secondaries from dicebox.
  if (nCap157Gd) {
    for (size_t lp1 = (*fSecondary).size() - numSecondaries; lp1 < (*fSecondary).size(); lp1++) {
      G4ParticleDefinition *particle = (*fSecondary)[lp1]->GetDefinition();
      G4String nameParticle = particle->GetParticleName();
      if (nameParticle != "Gd158") {
        (*fSecondary)[lp1]->SetTrackStatus(fPostponeToNextEvent);
      }
    }

    Dicebox158Gd diceboxObj;
    G4VParticleChange *myParticleChange = diceboxObj.GenericPostStepDoIt(aStep);
    G4int iSec = myParticleChange->GetNumberOfSecondaries();

    if (iSec > 0) {
      while ((iSec--) > 0) {
        G4Track *tempSecTrack = myParticleChange->GetSecondary(iSec);
        fpSteppingManager->GetfSecondary()->push_back(tempSecTrack);
      }
    }
    myParticleChange->Clear();
  }

#ifdef debug_dicebox158Gd
  const std::vector<const G4Track *> *secondary = aStep->GetSecondaryInCurrentStep();
  if (nameProcess == "nCapture") {
    RAT::debug << newline << "=======================AFTER DICEBOX======================" << newline;

    for (size_t lp = 0; lp < (*secondary).size(); lp++) {
      G4ParticleDefinition *par = (*secondary)[lp]->GetDefinition();

      RAT::debug << "(i,name,pos,momentum,erg,time) : (" << lp << "," << par->GetParticleName() << ","
             << (*secondary)[lp]->GetPosition() << "," << (*secondary)[lp]->GetMomentum() << ","
             << (*secondary)[lp]->GetKineticEnergy() << "," << (*secondary)[lp]->GetGlobalTime() << ")" << newline;

      RAT::debug << "(parentID,trackID,stepID,status) : (" << (*secondary)[lp]->GetParentID() << ","
             << (*secondary)[lp]->GetTrackID() << "," << (*secondary)[lp]->GetCurrentStepNumber() << ","
             << (*secondary)[lp]->GetTrackStatus() << ")" << newline;
    }
  }

#endif

  //===========================================================================//
  // END OF DICEBOX //
  //===========================================================================//

#ifdef G4DEBUG
  static G4Timer timer;

  timer.Stop();
  G4double dut = timer.GetUserElapsed();
  timer.Start();

  // check for NULL world volume
  if (track->GetVolume() == NULL) {
    const G4VProcess *lastproc = track->GetStep()->GetPostStepPoint()->GetProcessDefinedStep();
    G4cerr << "GLG4SteppingAction: Track in NULL volume, terminating!" << newline
           << " step_no=" << track->GetCurrentStepNumber() << " type=" << track->GetDefinition()->GetParticleName()
           << "\n volume=NULL"
           << " last_process=" << (lastproc != 0 ? lastproc->GetProcessName() : G4String("NULL"))
           << "\n position=" << track->GetPosition() << " momentum=" << track->GetMomentum() << newline;
    track->SetTrackStatus(fStopAndKill);
  }

  // check total energy deposit
  GLG4SteppingAction_totEdep += aStep->GetTotalEnergyDeposit();

  // debugging (timing) info
  static G4String lastParticleName;
  if (dut > 0.0) {
    G4String key;
    G4String particleName = track->GetDefinition()->GetParticleName();
    const G4VProcess *preProcess = aStep->GetPreStepPoint()->GetProcessDefinedStep(),
                     *postProcess = aStep->GetPostStepPoint()->GetProcessDefinedStep();
    if (preProcess) key += preProcess->GetProcessName();
    key += "_";
    if (postProcess) key += postProcess->GetProcessName();
    GLG4SteppingAction_times[key].sum(dut);
    GLG4SteppingAction_times[particleName].sum(dut);
    key += "_" + particleName;
    if (particleName != lastParticleName) {
      key += "_" + lastParticleName;
      lastParticleName = particleName;
    }
    GLG4SteppingAction_times[key].sum(dut);
  }
#endif

  // do scintillation photons, and also re-emission
  if (fUseGLG4) {
    // invoke scintillation process
    G4VParticleChange *pParticleChange = GLG4Scint::GenericPostPostStepDoIt(aStep);
    // were any secondaries defined?
    G4int iSecondary = pParticleChange->GetNumberOfSecondaries();
    if (iSecondary > 0) {
      // add secondaries to the list
      while ((iSecondary--) > 0) {
        G4Track *tempSecondaryTrack = pParticleChange->GetSecondary(iSecondary);
        fpSteppingManager->GetfSecondary()->push_back(tempSecondaryTrack);
      }
    }
    // clear ParticleChange
    pParticleChange->Clear();
  }

  // Commented out because this duplicates the function of GLG4DeferTrackProc
  // // if end step time since start of event is past event window, defer to
  // later G4TrackStatus status= track->GetTrackStatus(); if ( (status==fAlive
  // || status==fSuspend) &&
  //      aStep->GetPostStepPoint()->GetGlobalTime() >
  //      myGenerator->GetEventWindow() ) {
  //   myGenerator->DeferTrackToLaterEvent(track);
  //   track->SetTrackStatus(status=fStopAndKill);
  // }

  // Accumulate step statistics

  RAT::TrackInfo *trackInfo = dynamic_cast<RAT::TrackInfo *>(track->GetUserInformation());
  G4ThreeVector postStepPoint = aStep->GetPostStepPoint()->GetPosition();

  if (track->GetDefinition()->GetParticleName() != "opticalphoton") {
    trackInfo->energyCentroid.Fill(TVector3(postStepPoint.x(), postStepPoint.y(), postStepPoint.z()),
                                   aStep->GetTotalEnergyDeposit());
  }

  G4VPhysicalVolume *volume = aStep->GetPostStepPoint()->GetPhysicalVolume();
  if (volume) {
    std::string volumeName = volume->GetName();
    trackInfo->energyLoss[volumeName] += aStep->GetTotalEnergyDeposit();
  }

#ifdef G4DEBUG
  // Particle illumination map
  G4TrackStatus status = track->GetTrackStatus();
  if (status == fStopAndKill || status == fKillTrackAndSecondaries) {
    G4double sx, sy, sz;
    G4ThreeVector postPos = aStep->GetPostStepPoint()->GetPosition();
    sx = 0.5 + postPos.x() / widthIlluminationMap;
    sy = 0.5 + postPos.y() / widthIlluminationMap;
    sz = 0.5 + postPos.z() / widthIlluminationMap;
    if (sx > 0.0 && sy > 0.0 && sz > 0.0 && sx < 1.0 && sy < 1.0 && sz < 1.0) {
      static const G4Color defaultcolor(0.1, 0.1, 0.1);
      G4VPhysicalVolume *pv;
      const G4VisAttributes *att;
      const G4Color *c;
      (((pv = track->GetNextVolume()) || (pv = track->GetVolume())) &&
       (att = pv->GetLogicalVolume()->GetVisAttributes()) && (c = &(att->GetColor()))) ||
          (c = &defaultcolor);
      updateIlluminationMap(((sx < 0.5) ? 0 : 1), 1.0 - sz, sy, c);
      updateIlluminationMap(((sy < 0.5) ? 2 : 3), 1.0 - sz, sx, c);
      updateIlluminationMap(((sz < 0.5) ? 4 : 5), 1.0 - sy, sx, c);
    }
  }
  // reset timer; measure our own elapsed time
  timer.Stop();
  GLG4SteppingAction_internal_time += timer.GetUserElapsed();
  timer.Start();
#endif
}
