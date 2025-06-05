#include <CLHEP/Units/SystemOfUnits.h>
#include <math.h>
#include <sys/resource.h>

#include <RAT/AmBeGen.hh>
#include <RAT/BWVetGenericChamber.hh>
#include <RAT/BWVetGenericChamberHit.hh>
#include <RAT/CfGen.hh>
#include <RAT/Coincidence_Gen.hh>
#include <RAT/Config.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/DecayChain_Gen.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/EventInfo.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4DebugMessenger.hh>
#include <RAT/GLG4PMTOpticalModel.hh>
#include <RAT/GLG4PrimaryGeneratorAction.hh>
#include <RAT/GLG4Scint.hh>
#include <RAT/GLG4SteppingAction.hh>
#include <RAT/GLG4VertexGen.hh>
#include <RAT/GdGen.hh>
#include <RAT/Gen_LED.hh>
#include <RAT/GeoFiberSensitiveDetector.hh>
#include <RAT/GeoFiberSensitiveDetectorHit.hh>
#include <RAT/GeoNestedSolidArrayFactoryBase.hh>
#include <RAT/Gsim.hh>
#include <RAT/HeGen.hh>
#include <RAT/LiGen.hh>
#include <RAT/Log.hh>
#include <RAT/NGen.hh>
#include <RAT/PDFPMTCharge.hh>
#include <RAT/PDFPMTTime.hh>
#include <RAT/PMTFactoryBase.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/PrimaryVertexInformation.hh>
#include <RAT/ProcBlock.hh>
#include <RAT/ReacIBDgen.hh>
#include <RAT/RooTracker_Gen.hh>
#include <RAT/SNgen.hh>
#include <RAT/SignalHandler.hh>
#include <RAT/StackingAction.hh>
#include <RAT/TimeUtil.hh>
#include <RAT/TrackInfo.hh>
#include <RAT/Trajectory.hh>
#include <RAT/VertexFile_Gen.hh>
#include <RAT/VertexGen_CC.hh>
#include <RAT/VertexGen_CRY.hh>
#include <RAT/VertexGen_Decay0.hh>
#include <RAT/VertexGen_ES.hh>
#include <RAT/VertexGen_FastNeutron.hh>
#include <RAT/VertexGen_IBD.hh>
#include <RAT/VertexGen_Isotope.hh>
#include <RAT/VertexGen_ReacIBD.hh>
#include <RAT/VertexGen_SN.hh>
#include <RAT/VertexGen_Spectrum.hh>
#include <Randomize.hh>
#include <cstdlib>
#include <vector>

namespace RAT {

// Doesn't waste space unless you want to draw tracks
bool Gsim::FillPointCont = false;
bool Gsim::StoreOpticalTrackID = false;
double Gsim::MaxWallTime = 0;
std::set<G4String> Gsim::fStoreParticleTraj;
std::set<G4String> Gsim::fDiscardParticleTraj;

// 01-Aug-2006 WGS: I'm putting in the default initializers to remind
// us that they're being called anyway, because Gsim inherits from
// these classes.
Gsim::Gsim() : Producer(), G4UserRunAction(), G4UserEventAction(), G4UserTrackingAction() {
  mainBlock = NULL;
  Init();
}

Gsim::Gsim(ProcBlock *theMainBlock) : Producer(), G4UserEventAction(), G4UserTrackingAction() {
  mainBlock = theMainBlock;
  Init();
}

int get_pdgcode(const G4PrimaryParticle *p) {
  G4int glg4pdgcode = p->GetPDGcode();
  if (glg4pdgcode == 0 && p->GetG4code() != 0) {
    G4ParticleDefinition *pdef = p->GetG4code();
    if (G4IonTable::IsIon(pdef)) {
      int atomicNumber = G4int(pdef->GetPDGCharge() / CLHEP::eplus);
      int atomicMass = pdef->GetBaryonNumber();
      glg4pdgcode = GLG4VertexGen_HEPEvt::kIonCodeOffset + 1000 * atomicNumber + atomicMass;
    }
  }
  return glg4pdgcode;
}

void Gsim::Init() {
  // GEANT4 setup with GLG4sim detector construction and physics processes
  // Adapted from glg4sim.cc. Thanks Glenn!
  // Manages GEANT4 simulation process
  theRunManager = G4RunManager::GetRunManager();

  // Detector geometry
  DetectorConstruction *theDetectorConstruction = DetectorConstruction::GetDetectorConstruction();
  theRunManager->SetUserInitialization(theDetectorConstruction);

  // Particle generator
  theRunManager->SetUserAction(new GLG4PrimaryGeneratorAction());
  theRunManager->SetUserAction(new StackingAction);

  // Add RAT-specific generators
  GlobalFactory<GLG4VertexGen>::Register("ibd", new Alloc<GLG4VertexGen, VertexGen_IBD>);
  GlobalFactory<GLG4VertexGen>::Register("reacibd", new Alloc<GLG4VertexGen, VertexGen_ReacIBD>);
  GlobalFactory<GLG4VertexGen>::Register("es", new Alloc<GLG4VertexGen, VertexGen_ES>);
  GlobalFactory<GLG4VertexGen>::Register("cc", new Alloc<GLG4VertexGen, VertexGen_CC>);
  GlobalFactory<GLG4VertexGen>::Register("decay0", new Alloc<GLG4VertexGen, VertexGen_Decay0>);
  GlobalFactory<GLG4VertexGen>::Register("spectrum", new Alloc<GLG4VertexGen, VertexGen_Spectrum>);
  GlobalFactory<GLG4VertexGen>::Register("supernova", new Alloc<GLG4VertexGen, VertexGen_SN>);
  GlobalFactory<GLG4VertexGen>::Register("isotope", new Alloc<GLG4VertexGen, VertexGen_Isotope>);
  GlobalFactory<GLG4VertexGen>::Register("fastneutron", new Alloc<GLG4VertexGen, VertexGen_FastNeutron>);
#if CRY_Enabled
  GlobalFactory<GLG4VertexGen>::Register("cry", new Alloc<GLG4VertexGen, VertexGen_CRY>);
#endif

  GlobalFactory<GLG4Gen>::Register("decaychain", new Alloc<GLG4Gen, DecayChain_Gen>);
  GlobalFactory<GLG4Gen>::Register("cf", new Alloc<GLG4Gen, CfGen>);
  GlobalFactory<GLG4Gen>::Register("ambe", new Alloc<GLG4Gen, AmBeGen>);
  GlobalFactory<GLG4Gen>::Register("gd", new Alloc<GLG4Gen, GdGen>);
  GlobalFactory<GLG4Gen>::Register("li", new Alloc<GLG4Gen, LiGen>);
  GlobalFactory<GLG4Gen>::Register("n", new Alloc<GLG4Gen, NGen>);
  GlobalFactory<GLG4Gen>::Register("he", new Alloc<GLG4Gen, HeGen>);
  GlobalFactory<GLG4Gen>::Register("led", new Alloc<GLG4Gen, Gen_LED>);
  GlobalFactory<GLG4Gen>::Register("coincidence", new Alloc<GLG4Gen, Coincidence_Gen>);
  GlobalFactory<GLG4Gen>::Register("vertexfile", new Alloc<GLG4Gen, VertexFile_Gen>);
  GlobalFactory<GLG4Gen>::Register("rootracker", new Alloc<GLG4Gen, RooTracker_Gen>);

  // An additional "messenger" class for user diagnostics
  theDebugMessenger = new GLG4DebugMessenger(theDetectorConstruction);

  // User actions to control Run Manager behavior

  // Detect start of run so we can create run description objects
  theRunManager->SetUserAction(static_cast<G4UserRunAction *>(this));

  // Tracking action used to add our specialized Trajectory object
  // to capture particle track information
  theRunManager->SetUserAction(static_cast<G4UserTrackingAction *>(this));
  theRunManager->SetUserAction(new GLG4SteppingAction);

  // ...and finally the hook.  Here's where we trap GEANT4 at the end of
  // each event and do our business.
  theRunManager->SetUserAction(static_cast<G4UserEventAction *>(this));

  // PMT transit time and single-pe charge calculators
  fPMTTime.resize(0);
  fPMTCharge.resize(0);
}

Gsim::~Gsim() {
  // GEANT4 will try to delete the G4UserEventAction when we delete
  // the Run Manager, but that object is us!!  Clear event action
  // first to avoid circular delete.  Funny casting because
  // SetUserAction() is overloaded.
  theRunManager->SetUserAction(static_cast<G4UserEventAction *>(NULL));
  theRunManager->SetUserAction(static_cast<G4UserTrackingAction *>(NULL));

  for (size_t i = 0; i < fPMTTime.size(); i++) {
    delete fPMTTime[i];
    delete fPMTCharge[i];
  }
}

void Gsim::BeginOfRunAction(const G4Run * /*aRun*/) {
  DBLinkPtr lmc = DB::Get()->GetLink("MC");
  runID = DB::Get()->GetDefaultRun();
  utc = TTimeStamp();  // default to now

  info << "Gsim: Simulating run " << runID << newline;
  info << "Gsim: Run start at " << utc.AsString() << newline;

  if (!DS::RunStore::GetRun(runID)) {
    MakeRun(runID);
  }

  run = DS::RunStore::GetRun(runID);
  fPMTInfo = run->GetPMTInfo();
  fNestedTubeInfo = run->GetNestedTubeInfo();
  GLG4VEventAction::GetTheHitPMTCollection()->SetChannelStatus(run->GetChannelStatus());

  for (size_t i = 0; i < fPMTTime.size(); i++) {
    delete fPMTTime[i];
    delete fPMTCharge[i];
  }

  const size_t numModels = fPMTInfo->GetModelCount();
  fPMTTime.resize(numModels);
  fPMTCharge.resize(numModels);
  for (size_t i = 0; i < numModels; i++) {
    const std::string modelName = fPMTInfo->GetModelName(i);
    try {
      fPMTTime[i] = new RAT::PDFPMTTime(modelName);
    } catch (DBNotFoundError &e) {
      // fallback to default table if model is not available
      fPMTTime[i] = new RAT::PDFPMTTime();
    }
    try {
      fPMTCharge[i] = new RAT::PDFPMTCharge(modelName);
    } catch (DBNotFoundError &e) {
      // fallback to default table if model is not avaliable
      fPMTCharge[i] = new RAT::PDFPMTCharge();
    }
  }

  // Tell the generator when the run starts
  GLG4PrimaryGeneratorAction *theGLG4PGA = GLG4PrimaryGeneratorAction::GetTheGLG4PrimaryGeneratorAction();
  theGLG4PGA->SetRunUTC(utc);

  // Find out whether /tracking/storeTrajectory was set by user.
  // fpTrackingManager provided by G4UserTrackingAction parent class
  // We have to restore this state at the end of the event.
  fInitialStoreTrajectoryState = fpTrackingManager->GetStoreTrajectory();

  // Check for a maximum photoelectron limit. Events exceeding this limit
  // are aborted.
  try {
    maxpe = lmc->GetI("max_pe");
    warn << "Gsim: Aborting tracking for events exceeding " << maxpe << " photoelectrons" << newline;
  } catch (DBNotFoundError &e) {
    maxpe = 0;
  }
  nabort = 0;

  // Begin of run actions cascade
  mainBlock->BeginOfRun(run);
}

void Gsim::EndOfRunAction(const G4Run * /*arun*/) {
  if (maxpe > 0) {
    info << "Gsim: Tracking aborted for " << nabort << " events exceeding " << maxpe << " photoelectrons" << newline;
  }
  mainBlock->EndOfRun(run);
}

void Gsim::BeginOfEventAction(const G4Event *anEvent) {
  GLG4Scint::ResetTotEdep();
  GLG4Scint::ResetPhotonCount();

  // Clearing theHitPMTCollection clears away the HitPhotons and HitPMTs
  GLG4VEventAction::GetTheHitPMTCollection()->Clear();

  EventInfo *eventInfo = dynamic_cast<EventInfo *>(anEvent->GetUserInformation());

  eventInfo->StorePhotonIDs = StoreOpticalTrackID;

  // This is only necessary if the photons are flagged to be tracked and stored
  if (StoreOpticalTrackID) {
    OpticalPhotonIDs.resize(10000);
  }
}

void Gsim::EndOfEventAction(const G4Event *anEvent) {
  // Now build data structure out of G4Event
  DS::Root *ds = new DS::Root;
  MakeEvent(anEvent, ds);
  ds->SetRunID(runID);
  ds->AppendProcResult("gsim", Processor::OK);
  ds->SetRatVersion(RATVERSION);

  // Let main processor block process the event
  mainBlock->DSEvent(ds);

  delete ds;

  // Check if the user has requested exit
  if (SignalHandler::IsTermRequested()) {
    warn << "Terminating since Ctrl-C (SIGINT) caught..." << newline;
    theRunManager->AbortRun(true);  // Soft abort
  }

  // Reset the store trajectory state to the initial state as
  // specified in the user macro.
  fpTrackingManager->SetStoreTrajectory(fInitialStoreTrajectoryState);

  if (StoreOpticalTrackID) {
    OpticalPhotonIDs.resize(0);
  }

  // Soft-abort if a maximum runtime is set.
  if (MaxWallTime > 0) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double usertime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    if (usertime > MaxWallTime) {
      theRunManager->AbortRun(true);
    }
  }
}

void Gsim::PreUserTrackingAction(const G4Track *aTrack) {
  // Add storage for our custom tracking information if
  // we haven't already added it by creating a track object elsewhere
  // (like reemitted photons)
  if (!aTrack->GetUserInformation()) {
    // grumble, grumble, C++ const keyword, grumble
    const_cast<G4Track *>(aTrack)->SetUserInformation(new TrackInfo);
  }

  // For very large, complex tracks, it is not sufficient to
  // discard the track if we do not want it.  We must prevent
  // GEANT4 from saving the information in the first place.
  // To support this on a particle-by-particle basis, we have
  // to flip the state of track storage in the tracking manager
  // on the fly.  We restore the original state at the end of
  // the event, which is critical if beamOn is called more than once
  // in a macro.

  // Store track if requested for this particle.  fpTrackingManager
  // provided by G4UserTrackingAction parent class
  if (GetStoreParticleTraj(aTrack->GetDefinition()->GetParticleName())) {
    fpTrackingManager->SetStoreTrajectory(true);
    fpTrackingManager->SetTrajectory(new Trajectory(aTrack));
  } else {
    fpTrackingManager->SetStoreTrajectory(false);
  }

  if (aTrack->GetDefinition()->GetParticleName() == "opticalphoton") {
    G4Event *event = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
    EventInfo *eventInfo = dynamic_cast<EventInfo *>(event->GetUserInformation());
    TrackInfo *trackInfo = dynamic_cast<TrackInfo *>(aTrack->GetUserInformation());

    std::string creatorProcessName;
    const G4VProcess *creatorProcess = aTrack->GetCreatorProcess();
    if (creatorProcess) {
      creatorProcessName = creatorProcess->GetProcessName();
    }
    // Now deal with creator process naming override from trackInfo
    if (trackInfo && trackInfo->GetCreatorProcess() != "") {
      creatorProcessName = trackInfo->GetCreatorProcess();
    }

    if (creatorProcessName.find("Scintillation") != std::string::npos) {
      eventInfo->numScintPhoton++;
    } else if (creatorProcessName.find("Reemission") != std::string::npos) {
      eventInfo->numReemitPhoton++;
    } else if (creatorProcessName.find("Cerenkov") != std::string::npos) {
      eventInfo->numCerenkovPhoton++;
    }
  }
}

void Gsim::PostUserTrackingAction(const G4Track *aTrack) {
  // Now that we know how the track was terminated, we can fill
  // the OpticalCentroid
  std::string creatorProcessName;
  std::string destroyerProcessName;

  // The road to hell is paved with global variables,
  // and GEANT4 is my travelling companion.
  G4Event *event = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
  EventInfo *eventInfo = dynamic_cast<EventInfo *>(event->GetUserInformation());
  TrackInfo *trackInfo = dynamic_cast<TrackInfo *>(aTrack->GetUserInformation());

  // Abort events with too many PE if a limit has been set.
  if (maxpe > 0) {
    int nhits = 0;
    GLG4HitPMTCollection *hitpmts = GLG4VEventAction::GetTheHitPMTCollection();
    for (int ipmt = 0; ipmt < hitpmts->GetEntries(); ipmt++) {
      nhits += hitpmts->GetPMT(ipmt)->GetEntries();
    }
    if (nhits > maxpe) {
      G4EventManager::GetEventManager()->AbortCurrentEvent();
      event->SetEventAborted();
      nabort++;
    }
  }

  int TrackID = aTrack->GetTrackID();
  int ParentID = aTrack->GetParentID();

  // Determine the creator process
  const G4VProcess *creatorProcess = aTrack->GetCreatorProcess();
  if (creatorProcess) {
    creatorProcessName = creatorProcess->GetProcessName();
  }

  // Now deal with creator process naming override from trackInfo
  if (trackInfo && trackInfo->GetCreatorProcess() != "") {
    creatorProcessName = trackInfo->GetCreatorProcess();
  }

  if (trackInfo) {
    // Fill the energy loss by volume map
    for (std::map<std::string, double>::iterator it = trackInfo->energyLoss.begin(); it != trackInfo->energyLoss.end();
         it++) {
      eventInfo->energyLoss[it->first] = it->second;
    }

    // Fill the energy centroid
    eventInfo->energyCentroid.Add(trackInfo->energyCentroid);

    // Finally fill the optical centroid information if we have an optical
    // photon which was not created by the TPB but WAS absorbed by it. This
    // represents the best approximation to the "reconstructable" event
    // vertex, neglecting exotics like TPB scintillation and cerenkov emmission

    // With surface TPB model: Not reemitted, but killed by SurfaceAbsorption
    // With bulk TPB model:    Not made by OpWLS, but killed by OpWLS
    if ((aTrack->GetDefinition()->GetParticleName() == "opticalphoton") && (creatorProcessName != "Reemission") &&
        (creatorProcessName != "OpWLS")) {
      destroyerProcessName = aTrack->GetStep()->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
      if ((destroyerProcessName == "SurfaceAbsorption") || (destroyerProcessName == "OpWLS")) {
        G4ThreeVector startPosition = aTrack->GetVertexPosition();
        eventInfo->opticalCentroid.Fill(TVector3(startPosition.x(), startPosition.y(), startPosition.z()));
      }
    }
    if (StoreOpticalTrackID) {
      std::string particle_name = aTrack->GetDefinition()->GetParticleName();
      SetOpticalPhotonIDs(particle_name, TrackID, ParentID);
    }
  }
}

void Gsim::MakeRun(int _runID) {
  DBLinkPtr lrun = DB::Get()->GetLink("RUN", "", _runID);
  DS::Run *run = new DS::Run();

  run->SetID(_runID);
  run->SetType((unsigned)lrun->GetI("runtype"));
  run->SetStartTime(utc);
  run->SetNestedTubeInfo(&GeoNestedSolidArrayFactoryBase::GetNestedTubeInfo());
  const DS::PMTInfo *pmtinfo = &PMTFactoryBase::GetPMTInfo();
  run->SetPMTInfo(pmtinfo);
  DS::ChannelStatus ch_status;
  ch_status.Load(pmtinfo, lrun->GetS("channel_status"));
  run->SetChannelStatus(ch_status);

  DS::RunStore::AddNewRun(run);
}

void Gsim::MakeEvent(const G4Event *g4ev, DS::Root *ds) {
  DS::MC *mc = ds->GetMC();
  EventInfo *exinfo = dynamic_cast<EventInfo *>(g4ev->GetUserInformation());

  // Event Header
  ds->SetRunID(theRunManager->GetCurrentRun()->GetRunID());
  mc->SetID(g4ev->GetEventID());
  mc->SetUTC(exinfo->utc);
  // Vertex Info
  for (int ivert = 0; ivert < g4ev->GetNumberOfPrimaryVertex(); ivert++) {
    G4PrimaryVertex *pv = g4ev->GetPrimaryVertex(ivert);

    double t = pv->GetT0();
    TVector3 pos(pv->GetX0(), pv->GetY0(), pv->GetZ0());

    for (int ipart = 0; ipart < pv->GetNumberOfParticle(); ipart++) {
      DS::MCParticle *rat_mcpart = mc->AddNewMCParticle();

      G4PrimaryParticle *p = pv->GetPrimary(ipart);
      rat_mcpart->SetPDGCode(get_pdgcode(p));
      if (p->GetG4code()) {
        rat_mcpart->SetParticleName(p->GetG4code()->GetParticleName());
      } else {
        rat_mcpart->SetParticleName("NotDefined");
      }
      rat_mcpart->SetMomentum(TVector3(p->GetPx(), p->GetPy(), p->GetPz()));
      rat_mcpart->SetKE(sqrt(p->GetMass() * p->GetMass() + p->GetMomentum().mag2()) - p->GetMass());
      rat_mcpart->SetTime(t);
      rat_mcpart->SetPosition(pos);
      rat_mcpart->SetPolarization(TVector3(p->GetPolX(), p->GetPolY(), p->GetPolZ()));
      // Track end point info
      int track_id = p->GetTrackID();
      if (trackEndMap.find(track_id) == trackEndMap.end()) continue;
      std::vector<double> end_info = trackEndMap[track_id];
      rat_mcpart->SetEndPosition(TVector3(end_info[0], end_info[1], end_info[2]));
      rat_mcpart->SetEndTime(end_info[3]);
      rat_mcpart->SetEndMomentum(TVector3(end_info[4], end_info[5], end_info[6]));
      rat_mcpart->SetEndKE(end_info[7]);
    }

    PrimaryVertexInformation *ratpvi = dynamic_cast<PrimaryVertexInformation *>(pv->GetUserInformation());
    if (ratpvi) {
      for (int i = 0; i < ratpvi->GetParentParticleCount(); i++) {
        G4PrimaryParticle *p = ratpvi->GetParentParticle(i);
        DS::MCParticle *rat_mcparent = mc->AddNewMCParent();
        rat_mcparent->SetPDGCode(get_pdgcode(p));
        if (p->GetG4code()) {
          rat_mcparent->SetParticleName(p->GetG4code()->GetParticleName());
        } else {
          rat_mcparent->SetParticleName("NotDefined");
        }
        rat_mcparent->SetMomentum(TVector3(p->GetPx(), p->GetPy(), p->GetPz()));
        rat_mcparent->SetKE(sqrt(p->GetMass() * p->GetMass() + p->GetMomentum().mag2()) - p->GetMass());
        rat_mcparent->SetTime(t);
        rat_mcparent->SetPosition(pos);
        rat_mcparent->SetPolarization(TVector3(p->GetPolX(), p->GetPolY(), p->GetPolZ()));
      }
    }
  }

  // Calibration source information
  const DS::Calib *calib = exinfo->GetCalib();
  // only copy if there is something real to copy
  if (calib->GetID() != -1) {
    *ds->GetCalib() = *calib;
  }

  // Trajectory Info
  G4TrajectoryContainer *traj_list = g4ev->GetTrajectoryContainer();
  if (traj_list) {
    for (size_t itraj = 0; itraj < traj_list->size(); itraj++) {
      Trajectory *traj = dynamic_cast<Trajectory *>((*traj_list)[itraj]);
      // Trajectory potentially cut during generation to save space
      if (!traj) {
        continue;
      }
      DS::MCTrack *track = mc->AddNewMCTrack();
      *track = *traj->GetTrack();
    }
  }

  // If the event was aborted because of the maximum number of
  // photoelectron limit, then store the information about the
  // primary particle and any tracking information, but do not keep
  // the PMT information.
  if (g4ev->IsAborted()) {
    return;
  }

  // MC summary information
  DS::MCSummary *summary = mc->GetMCSummary();
  summary->SetEnergyCentroid(exinfo->energyCentroid.GetMean());
  summary->SetEnergyRMS(exinfo->energyCentroid.GetRMS());
  summary->SetEnergyLossByVolume(exinfo->energyLoss);
  summary->SetTotalScintEdep(GLG4Scint::GetTotEdep());
  summary->SetTotalScintEdepQuenched(GLG4Scint::GetTotEdepQuenched());
  const G4ThreeVector sCentroid = GLG4Scint::GetScintCentroidSum();  // Normalized despite function name
  TVector3 scintCentroid(sCentroid.x(), sCentroid.y(), sCentroid.z());
  summary->SetTotalScintCentroid(scintCentroid);
  summary->SetNumScintPhoton(exinfo->numScintPhoton);
  summary->SetNumReemitPhoton(exinfo->numReemitPhoton);
  summary->SetNumCerenkovPhoton(exinfo->numCerenkovPhoton);
  // summary->SetPMTPhotonInfo(GLG4PMTOpticalModel::pmtHitVector);

  // GLG4Scint::ResetTimeChargeMatrix();
  exinfo->timePhotonMatrix.resize(0);
  // GLG4PMTOpticalModel::pmtHitVector.resize(0);

  /** PMT simulation */
  GLG4HitPMTCollection *hitpmts = GLG4VEventAction::GetTheHitPMTCollection();
  int numPE = 0;

  for (int ipmt = 0; ipmt < hitpmts->GetEntries(); ipmt++) {
    GLG4HitPMT *a_pmt = hitpmts->GetPMT(ipmt);
    a_pmt->SortTimeAscending();

    // Create and initialize a RAT DS::MCPMT
    // note that GLG4HitPMTs are given IDs which are their index
    DS::MCPMT *rat_mcpmt = mc->AddNewMCPMT();
    // the index of the PMT we just added
    rat_mcpmt->SetID(a_pmt->GetID());
    rat_mcpmt->SetType(fPMTInfo->GetType(a_pmt->GetID()));

    numPE += a_pmt->GetEntries();
    /** Add "real" hits from actual simulated photons */
    for (int i = 0; i < a_pmt->GetEntries(); i++) {
      // Find the optical process responsible
      auto photon = a_pmt->GetPhoton(i);
      std::string process = photon->GetCreatorProcess();
      if (StoreOpticalTrackID) {
        AddMCPhoton(rat_mcpmt, a_pmt->GetPhoton(i), exinfo, process);
      } else {
        AddMCPhoton(rat_mcpmt, a_pmt->GetPhoton(i), NULL, process);
      }
    }
  }
  mc->SetNumPE(numPE);

  /** hits from fibers */
  G4HCofThisEvent *HC = g4ev->GetHCofThisEvent();
  for (int hc = 0; hc < HC->GetNumberOfCollections(); hc++) {
    GeoFiberSensitiveDetectorHitsCollection *hit_collection = (GeoFiberSensitiveDetectorHitsCollection *)HC->GetHC(hc);
    if (hit_collection->GetName() != "FiberSenDet" || hit_collection->GetSize() == 0) continue;
    DS::MCNestedTube *rat_mcnt = mc->AddNewMCNestedTube();
    G4String det_name = hit_collection->GetSDname();
    std::string fibre_id_str = det_name.erase(0, 6).data();
    int fibre_id = std::stoi(fibre_id_str);
    rat_mcnt->SetID(fibre_id);
    // only process fibers
    // info << hit_collection->GetSDname() << newline;
    for (size_t hit = 0; hit < hit_collection->GetSize(); hit++) {
      GeoFiberSensitiveDetectorHit *my_hit = (GeoFiberSensitiveDetectorHit *)hit_collection->GetHit(hit);
      AddMCNestedTubeHit(rat_mcnt, my_hit);
    }
  }
}

void Gsim::AddMCPhoton(DS::MCPMT *rat_mcpmt, const GLG4HitPhoton *photon, EventInfo * /*exinfo*/, std::string process) {
  DS::MCPhoton *rat_mcphoton = rat_mcpmt->AddNewMCPhoton();
  double chargeScale = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetChargeScaleByPMTID(rat_mcpmt->GetID());
  // Only real photons are added in Gsim, noise and afterpulsing handled in processors
  rat_mcphoton->SetDarkHit(false);
  rat_mcphoton->SetAfterPulse(false);
  rat_mcphoton->SetLambda(photon->GetWavelength());

  double x, y, z;
  photon->GetPosition(x, y, z);
  rat_mcphoton->SetPosition(TVector3(x, y, z));
  photon->GetMomentum(x, y, z);
  rat_mcphoton->SetMomentum(TVector3(x, y, z));
  photon->GetPolarization(x, y, z);
  rat_mcphoton->SetPolarization(TVector3(x, y, z));

  rat_mcphoton->SetTrackID(photon->GetTrackID());
  rat_mcphoton->SetHitTime(photon->GetTime());
  rat_mcphoton->SetCreationTime(photon->GetCreationTime());

  rat_mcphoton->SetFrontEndTime(fPMTTime[fPMTInfo->GetModel(rat_mcpmt->GetID())]->PickTime(photon->GetTime()));
  // Set the charge for the photoelectron, scaled by an optional calibration parameter chargeScale with a default value
  // of one
  rat_mcphoton->SetCharge(chargeScale * fPMTCharge[fPMTInfo->GetModel(rat_mcpmt->GetID())]->PickCharge());
  rat_mcphoton->SetCreatorProcess(process);
}

void Gsim::AddMCNestedTubeHit(DS::MCNestedTube *rat_mcnt, const GeoFiberSensitiveDetectorHit *hit) {
  DS::MCNestedTubeHit *rat_mchit = rat_mcnt->AddNewMCNestedTubeHit();
  // Only real photons are added in Gsim, noise and afterpulsing handled in processors

  double x, y, z;
  G4ThreeVector pos_vec = hit->GetHitPos();
  x = pos_vec.x();
  y = pos_vec.y();
  z = pos_vec.z();
  rat_mchit->SetPosition(TVector3(x, y, z));

  rat_mchit->SetHitID(hit->GetID());
  rat_mchit->SetHitTime(hit->GetTime());
}

void Gsim::SetStoreParticleTraj(const G4String &particleName, const bool &gDoStore) {
  if (gDoStore) {
    fStoreParticleTraj.insert(particleName);
  } else {
    fDiscardParticleTraj.insert(particleName);
  }
}

bool Gsim::GetStoreParticleTraj(const G4String &particleName) {
  if (!fInitialStoreTrajectoryState) {
    // in case fStoreParticleTraj has an entry, keep
    // track only if particle name is in fStoreParticleTraj
    if (!fStoreParticleTraj.empty()) {
      std::set<G4String>::const_iterator it = fStoreParticleTraj.find(particleName);
      if (it == fStoreParticleTraj.end())
        return false;
      else
        return true;
    } else
      return false;
  } else {
    // check whether request was made to throw these tracks away by checking
    // whether this particleName exists in fDiscardParticleTraj
    std::set<G4String>::const_iterator it = fDiscardParticleTraj.find(particleName);
    if (it == fDiscardParticleTraj.end())
      return true;
    else
      return false;
  }
}

G4String Gsim::GetStoreParticleTrajString(const bool &gDoStore) {
  G4String ret = "";
  if (gDoStore) {
    for (std::set<G4String>::const_iterator it = fStoreParticleTraj.begin(); it != fStoreParticleTraj.end(); it++) {
      ret += *it;
      ret += " ";
    }
  } else {
    for (std::set<G4String>::const_iterator it = fStoreParticleTraj.begin(); it != fStoreParticleTraj.end(); it++) {
      ret += *it;
      ret += " ";
    }
  }
  return ret;
}

// PhotonRecurse runs back from the PMT photon through the track - parent
// ID pairs in PhotonIDs to find the original track which created the
// optical photon
void Gsim::PhotonRecurse(std::vector<int> &PhotonIDs, int trackID, int &parentID, int &firstCreatedID) {
  if (PhotonIDs[trackID] == 0 || PhotonIDs[trackID] == -1) {
    parentID = trackID;
    return;
  }

  if (PhotonIDs[trackID] != 0) firstCreatedID = trackID;

  PhotonRecurse(PhotonIDs, PhotonIDs[trackID], parentID, firstCreatedID);
}

void Gsim::SetOpticalPhotonIDs(std::string particle_type, int trackID, int parentID) {
  if (static_cast<size_t>(trackID) > OpticalPhotonIDs.size()) {
    // this factor of 2 seems pretty random.
    OpticalPhotonIDs.resize(2 * trackID, 0);
  }
  if (particle_type.compare("opticalphoton") == 0) {
    OpticalPhotonIDs[trackID] = parentID;
  }
  if (particle_type.compare("opticalphoton") != 0) {
    OpticalPhotonIDs[trackID] = 0;
  }
}

}  // namespace RAT
