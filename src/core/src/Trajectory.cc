#include <G4StepPoint.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VProcess.hh>
#include <RAT/Gsim.hh>
#include <RAT/Log.hh>
#include <RAT/TrackInfo.hh>
#include <RAT/Trajectory.hh>
#include <RAT/NaiveQuenchingCalculator.hh>
#include <RAT/IntegratedQuenchingCalculator.hh>
#include <RAT/FixedTrapezoidalQuadrature.hh>
#include <RAT/AdaptiveSimpsonQuadrature.hh>


namespace RAT {

G4Allocator<Trajectory> aTrajectoryAllocator;
bool Trajectory::fgDoAppendMuonStepSpecial = false;
QuenchingCalculator* BuildQuenchingCalculator();

Trajectory::Trajectory() : G4Trajectory() {
  ratTrack = new DS::MCTrack;
  this->fQuenching = BuildQuenchingCalculator();
}

Trajectory::Trajectory(const G4Track *aTrack) : G4Trajectory(aTrack), creatorProcessName("start") {
  ratTrack = new DS::MCTrack;
  this->fQuenching = BuildQuenchingCalculator();

  ratTrack->SetID(GetTrackID());
  ratTrack->SetParentID(GetParentID());
  ratTrack->SetPDGCode(GetPDGEncoding());
  ratTrack->SetParticleName(GetParticleName());

  const G4VProcess *creatorProcess = aTrack->GetCreatorProcess();
  if (creatorProcess) creatorProcessName = creatorProcess->GetProcessName();
  // override with extra TrackInfo if present
  const TrackInfo *trackInfo = dynamic_cast<TrackInfo *>(aTrack->GetUserInformation());
  if (trackInfo && trackInfo->GetCreatorProcess() != "") creatorProcessName = trackInfo->GetCreatorProcess();

  ratTrack->SetLength(0.0);
  ratTrack->SetDepositedEnergy(0.0);
  ratTrack->SetScintEdepQuenched(0.0);
}

Trajectory::~Trajectory() {
  delete ratTrack;
  delete fQuenching;
}

QuenchingCalculator* BuildQuenchingCalculator() {
  QuenchingCalculator* quenching_calculator;

  RAT::DB *db = RAT::DB::Get();
  RAT::DBLinkPtr tbl = db->GetLink("QUENCHING");
  std::string selection = tbl->GetS("model");
  BirksLaw model;
  if (selection == "birks") {
    model = BirksLaw();
  } else {
    // no such quenching model
    std::string msg = "Invalid quenching model: " + selection;
    RAT::Log::Die(msg);
  }
  std::string strategy = tbl->GetS("strategy");
  if (strategy == "naive") {
    quenching_calculator = new NaiveQuenchingCalculator(model);
  } else if (strategy == "integrated") {
    std::string method = tbl->GetS("integration");
    Quadrature *quadrature;
    if (method == "fixed") {
      // TODO
      double resolution = tbl->GetD("resolution");
      quadrature = new FixedTrapezoidalQuadrature(resolution);
    } else if (method == "adaptive") {
      double tolerance = tbl->GetD("tolerance");
      quadrature = new AdaptiveSimpsonQuadrature(tolerance);
    } else {
      // no such integration method
      std::string msg = "Invalid integration method: " + method;
      RAT::Log::Die(msg);
    }
    quenching_calculator = new IntegratedQuenchingCalculator(model, quadrature);
  } else {
    // no such quenching calculation strategy
    std::string msg = "Invalid quenching calculation strategy: " + strategy;
    RAT::Log::Die(msg);
  }
  return quenching_calculator;
}

void Trajectory::AppendStep(const G4Step *aStep) {
  if (ratTrack->GetMCTrackStepCount() == 0) {
    // Add initial step at very beginning of track
    DS::MCTrackStep *initStep = ratTrack->AddNewMCTrackStep();
    G4StepPoint *initPoint = aStep->GetPreStepPoint();
    FillStep(initPoint, aStep, initStep, 0.0, true);
  }

  // Check if we are storing truncated stepping info for muons
  G4String particleName = aStep->GetTrack()->GetDefinition()->GetParticleName();
  if (fgDoAppendMuonStepSpecial == true && (particleName == "mu-" || particleName == "mu+")) {
    G4String processName = aStep->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
    // would also like to store steps where neutron was created...but not yet
    if (processName == "Transportation") {
      DS::MCTrackStep *ratStep = ratTrack->AddNewMCTrackStep();
      G4StepPoint *endPoint = aStep->GetPostStepPoint();
      FillStep(endPoint, aStep, ratStep, aStep->GetStepLength(), false);
      // Update total track length
      // ratTrack->SetLength(ratTrack->GetLength() + ratStep->GetLength());
      // previous line won't work if we're only keeping a subset of steps
      // so let's just set it to -1 for now to prevent misuse of
      // ratTrack->GetLength() instead, will have to use position of steps to
      // determine length
      ratTrack->SetLength(-1.);
      ratTrack->SetDepositedEnergy(-1.);
      ratTrack->SetScintEdepQuenched(-1.);
    }
    return;
  }

  DS::MCTrackStep *ratStep = ratTrack->AddNewMCTrackStep();
  G4StepPoint *endPoint = aStep->GetPostStepPoint();
  FillStep(endPoint, aStep, ratStep, aStep->GetStepLength(), false);
  // Update total track length
  ratTrack->SetLength(ratTrack->GetLength() + ratStep->GetLength());
  ratTrack->SetDepositedEnergy(ratTrack->GetDepositedEnergy() + ratStep->GetDepositedEnergy());
  ratTrack->SetScintEdepQuenched(ratTrack->GetScintEdepQuenched() + ratStep->GetScintEdepQuenched());
  if (Gsim::GetFillPointCont()) G4Trajectory::AppendStep(aStep);
}

void Trajectory::FillStep(const G4StepPoint *point, const G4Step *step, DS::MCTrackStep *ratStep, double stepLength,
                          bool isInit) {
  G4StepPoint *startPoint = step->GetPreStepPoint();

  ratStep->SetLength(stepLength);

  const G4ThreeVector &pos = point->GetPosition();
  ratStep->SetEndpoint(TVector3(pos.x(), pos.y(), pos.z()));
  ratStep->SetGlobalTime(point->GetGlobalTime());
  ratStep->SetLocalTime(point->GetLocalTime());
  ratStep->SetProperTime(point->GetProperTime());

  G4ThreeVector mom = point->GetMomentum();
  ratStep->SetMomentum(TVector3(mom.x(), mom.y(), mom.z()));
  ratStep->SetKE(point->GetKineticEnergy());

  if (isInit) {
    ratStep->SetDepositedEnergy(0);
    ratStep->SetScintEdepQuenched(0);
  } else {
    ratStep->SetDepositedEnergy(step->GetTotalEnergyDeposit());

    // Set step quenched energy, checking if the Birk's constant is set for the material
    RAT::DB *db = RAT::DB::Get();
    RAT::DBLinkPtr optics_tbl = db->GetLink("OPTICS", startPoint->GetMaterial()->GetName()); // get OPTICS table for current material
    double birksConstant = 0.0; // kB = 0  =>  no quenching
    try { // the Birk's constant (SCINTMOD_value2) may not be defined for the material
      std::vector<double> scintmod1_array = optics_tbl->GetDArray("SCINTMOD_value1");
      std::vector<double> scintmod2_array = optics_tbl->GetDArray("SCINTMOD_value2");
      for (size_t i=0; i<scintmod1_array.size(); i++) {
        if (scintmod1_array[i] == 1.0) {
          birksConstant = scintmod2_array[i];
        }
      }
      ratStep->SetScintEdepQuenched(fQuenching->QuenchedEnergyDeposit(*step, birksConstant));
    } catch (DBNotFoundError &e) {
      ratStep->SetScintEdepQuenched(step->GetTotalEnergyDeposit());
    }
  }

  const G4VProcess *process = point->GetProcessDefinedStep();
  if (process == 0)
    ratStep->SetProcess(creatorProcessName);  // Assume first step
  else
    ratStep->SetProcess(process->GetProcessName());

  G4VPhysicalVolume *volume = startPoint->GetPhysicalVolume();
  if (volume == NULL) {
    detail << "\nTrajectory encountered a NULL volume.  Continuing...\n";
    ratStep->SetVolume("NULL");
  } else {
    ratStep->SetVolume(volume->GetName());
  }
}

void Trajectory::MergeTrajectory(G4VTrajectory *secondTrajectory) {
  G4Trajectory::MergeTrajectory(secondTrajectory);

  Trajectory *secondTraj = dynamic_cast<Trajectory *>(secondTrajectory);
  if (secondTraj) {
    for (int i = 1; i < secondTraj->ratTrack->GetMCTrackStepCount(); i++)
      *ratTrack->AddNewMCTrackStep() = *secondTraj->ratTrack->GetMCTrackStep(i);
    ratTrack->SetLength(ratTrack->GetLength() + secondTraj->ratTrack->GetLength());
    ratTrack->SetDepositedEnergy(ratTrack->GetDepositedEnergy() + secondTraj->ratTrack->GetDepositedEnergy());
    ratTrack->SetScintEdepQuenched(ratTrack->GetScintEdepQuenched() + secondTraj->ratTrack->GetScintEdepQuenched());
    secondTraj->ratTrack->PruneMCTrackStep();
  }
}

}  // namespace RAT
