/** @file GLG4Scint.cc
    For GLG4Scint class, providing advanced scintillation process.
    Distantly based on an extensively modified version of G4Scintillation.cc.

    This file is part of the GenericLAND software library.
    $Id$

    @author Glenn Horton-Smith (Tohoku) 28-Jan-1999
*/
////////////////////////////////////////////////////////////////////////
/// REVISION HISTORY:\n
///         2012-06-30 : Nuno Barros - corrected compilation errors in DEBUG mode. \n
///         2015-06-25 : Ben Land - increased max possible scintillation time. \n
///         2016-11-07 : Anthony LaTorre - added rise time to scintillator time distribution. \n
///         2022-08-18 : Ed Callaghan - abstracted quenching calculaton to resolve
///                                     step-size-based inconsistency. \n
///         2026-03-31 : Logan Lebanowski - added multiple emitting components,
///                                         multi-photon re-emission via REEMISSION_MULT,
///                                         material definition logic control. \n
////////////////////////////////////////////////////////////////////////

// [see detailed class description in GLG4Scint.hh]

#include "RAT/GLG4Scint.hh"

#include <RAT/AdaptiveSimpsonQuadrature.hh>
#include <RAT/DB.hh>
#include <RAT/FixedTrapezoidalQuadrature.hh>
#include <RAT/IntegratedQuenchingCalculator.hh>
#include <RAT/Log.hh>
#include <RAT/NaiveQuenchingCalculator.hh>
#include <RAT/PhotonThinning.hh>
#include <RAT/TrackInfo.hh>
#include <fileio.hpp>

#include "G4Timer.hh"
#include "G4TrackFastVector.hh"  // for G4TrackFastVectorSize
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"
#include "G4ios.hh"
#include "Randomize.hh"

std::vector<GLG4Scint *> GLG4Scint::fMasterVectorOfGLG4Scint;
// top level of scintillation command
G4UIdirectory *GLG4Scint::fGLG4ScintDir = 0;

// universal maximum number of secondary tracks per step for GLG4Scint
// G4int GLG4Scint::maxTracksPerStep = G4TrackFastVectorSize;
G4int GLG4Scint::fMaxTracksPerStep = 180000;

// universal mean number of true photons per secondary track in GLG4Scint
G4double GLG4Scint::fMeanPhotonsPerSecondary = 1.0;

// universal on/off flag
G4bool GLG4Scint::fDoScintillation = true;
G4bool GLG4Scint::fDoReemission = true;

// energy deposition
G4double GLG4Scint::fTotEdep = 0.0;
G4double GLG4Scint::fTotEdepQuenched = 0.0;
G4double GLG4Scint::fTotEdepTime = 0.0;
G4ThreeVector GLG4Scint::fScintCentroidSum(0.0, 0.0, 0.0);

// default Quenching Factor
G4double GLG4Scint::fQuenchingFactor = 1.0;
// user-given (constant) quenching factor flag
G4bool GLG4Scint::fUserQF = false;
// default primary particle
G4String GLG4Scint::fPrimaryName = G4String();

G4double GLG4Scint::fPrimaryEnergy = 0.0;

GLG4DummyProcess GLG4Scint::fScintProcess("Scintillation", fUserDefined);
GLG4DummyProcess GLG4Scint::fReemissionProcess("Reemission", fUserDefined);
std::list<GLG4DummyProcess *> GLG4Scint::fEmissionProcessVector;
std::list<GLG4DummyProcess *> GLG4Scint::fReemissionProcessVector;

unsigned int GLG4Scint::fsScintillatedCount = 0;
unsigned int GLG4Scint::fsReemittedCount = 0;

/////////////////
// Constructors
/////////////////

GLG4Scint::GLG4Scint(const G4String &tablename, G4double lowerMassLimit) {
  fVerboseLevel = 0;
  fLowerMassLimit = lowerMassLimit;

  fMyPhysicsTable = MyPhysicsTable::FindOrBuild(tablename);
  fMyPhysicsTable->IncUsedBy();
  if (fVerboseLevel) {
    fMyPhysicsTable->Dump();
  }

  // add to ordered list
  if (fMasterVectorOfGLG4Scint.size() == 0 || lowerMassLimit >= fMasterVectorOfGLG4Scint.back()->fLowerMassLimit) {
    fMasterVectorOfGLG4Scint.push_back(this);
  } else {
    for (std::vector<GLG4Scint *>::iterator i = fMasterVectorOfGLG4Scint.begin(); i != fMasterVectorOfGLG4Scint.end();
         i++) {
      if (lowerMassLimit < (*i)->fLowerMassLimit) {
        fMasterVectorOfGLG4Scint.insert(i, this);
        break;
      }
    }
  }

  // create UI commands if necessary
  if (fGLG4ScintDir == nullptr) {
    // the scintillation control commands
    new G4UIdirectory("/rat/physics/");
    fGLG4ScintDir = new G4UIdirectory("/rat/physics/scintillation/");
    fGLG4ScintDir->SetGuidance("scintillation process control.");
    G4UIcommand *cmd;
    cmd = new G4UIcommand("/rat/physics/scintillation/on", this);
    cmd->SetGuidance("Turn on scintillation");
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/off", this);
    cmd->SetGuidance("Turn off scintillation");
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/reemission", this);
    cmd->SetGuidance("Turn on/off reemission of absorbed opticalphotons");
    cmd->SetParameter(new G4UIparameter("status", 's', false));
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/maxTracksPerStep", this);
    cmd->SetGuidance(
        "Set maximum number of opticalphoton tracks per step\n"
        "(If more real photons are needed, "
        "weight of tracked particles is increased.)\n");
    cmd->SetParameter(new G4UIparameter("maxTracksPerStep", 'i', false));
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/meanPhotonsPerSecondary", this);
    cmd->SetGuidance("Set mean number of \"real\" photons per secondary\n");
    cmd->SetParameter(new G4UIparameter("meanPhotonsPerSecondary", 'd', false));
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/verbose", this);
    cmd->SetGuidance("Set verbose level");
    cmd->SetParameter(new G4UIparameter("level", 'i', false));
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/dump", this);
    cmd->SetGuidance("Dump tables");
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
    cmd = new G4UIcommand("/rat/physics/scintillation/setQF", this);
    cmd->SetGuidance("Set a constant quenching factor, default is 1");
    cmd->SetParameter(new G4UIparameter("QuenchingFactor", 'd', false));
    cmd->AvailableForStates(G4State_Idle, G4State_GeomClosed, G4State_EventProc);
  }

#ifdef RATVERBOSE
  RAT::detail << "GLG4Scint[" << tablename << "]"
              << " is created " << newline;
#endif

  // ejc
  // open up db
  // if quenching selection is "naive", then QC = new NQC
  // if quenching selection is "integrated", then
  //    if integration is "fixed", then
  //        Quadrature = FixedQuadrature(resolution)
  //    if integration is "adaptive", then
  //        Quadrature = AdaptiveQuadrature(tolerance)
  //    QC = new IQC(Quadrature)
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
    this->fQuenching = new NaiveQuenchingCalculator(model);
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
    this->fQuenching = new IntegratedQuenchingCalculator(model, quadrature);
  } else {
    // no such quenching calculation strategy
    std::string msg = "Invalid quenching calculation strategy: " + strategy;
    RAT::Log::Die(msg);
  }
}

// GLG4Scint::GLG4Scint(const GLG4Scint &right)
// {
// }

////////////////
// Destructors
////////////////

GLG4Scint::~GLG4Scint() {
  fMyPhysicsTable->DecUsedBy();
  for (std::vector<GLG4Scint *>::iterator i = fMasterVectorOfGLG4Scint.begin(); i != fMasterVectorOfGLG4Scint.end();
       i++) {
    if (*i == this) {
      fMasterVectorOfGLG4Scint.erase(i);
      break;
    }
  }

  delete fQuenching;
}

////////////
// Methods
////////////

// Sets the quenching factor
void GLG4Scint::SetQuenchingFactor(G4double qf = 1.0) { fQuenchingFactor = qf; }

// PostStepDoIt
// -------------
//

#ifdef RATDEBUG
G4double GLG4Scint_tottime = 0.0;
G4int GLG4Scint_num_calls = 0;
G4int GLG4Scint_num_phots = 0;
#endif

// This routine is called for each step of any particle
// in a scintillator.  For accurate energy deposition, must be called
// from user-supplied UserSteppingAction, which also must stack
// any particles created.  A pseudo-Poisson-distributed number of
// photons is generated according to the scintillation yield formula,
// distributed evenly along the track segment and uniformly into 4pi.
G4VParticleChange *GLG4Scint::PostPostStepDoIt(const G4Track &aTrack, const G4Step &aStep) {
#ifdef RATDEBUG
  G4Timer timer;
  timer.Start();
  GLG4Scint_num_calls++;
#endif
  G4bool flagReemission = false;
  {
    // prepare to generate an event, organizing to
    // check for things that cause an early exit.
    fAParticleChange.Initialize(aTrack);
    const G4Material *aMaterial = aTrack.GetMaterial();
    const MyPhysicsTable::Entry *physicsEntry = fMyPhysicsTable->GetEntry(aMaterial->GetIndex());

    if (aTrack.GetDefinition() == G4OpticalPhoton::OpticalPhoton()) {
      flagReemission = fDoReemission && aTrack.GetTrackStatus() == fStopAndKill &&
                       aStep.GetPostStepPoint()->GetStepStatus() != fGeomBoundary;
      if (!flagReemission) {
        goto PostStepDoIt_DONE;
      }
    }

    G4double TotalEnergyDeposit = aStep.GetTotalEnergyDeposit();
    if (TotalEnergyDeposit <= 0.0 && !flagReemission) {
      goto PostStepDoIt_DONE;
    }

    // get pointer to the physics entry
    if (!physicsEntry) {
      goto PostStepDoIt_DONE;
    }

    // finds E-dependent QF, unless the user provided an E-independent one
    if (!fUserQF && *(fMyPhysicsTable->fName) == fPrimaryName) {
      if (physicsEntry->fQuenchingArray) {
        for (unsigned int iEntry = 0; iEntry < physicsEntry->fQuenchingArray->GetVectorLength(); iEntry++) {
          // preparations
          G4double CurrentEnergy = physicsEntry->fQuenchingArray->Energy(iEntry);
          G4double PreviousEnergy = CurrentEnergy;
          G4double PrimEn = GetPrimaryEnergy();
          G4double slope;
          // if the primary is below the energy range with a QF, then use QF of
          // lowest energy
          if (PrimEn < CurrentEnergy) {
            SetQuenchingFactor(physicsEntry->fQuenchingArray->Value(CurrentEnergy));
          } else {  // find 1st energy above primary, if available
            while ((PrimEn > CurrentEnergy) && iEntry++) {
              PreviousEnergy = CurrentEnergy;
              CurrentEnergy = physicsEntry->fQuenchingArray->Energy(iEntry);
            }
            // if primary energy above range or in quenching array, use QF of
            // last energy
            if (PrimEn >= CurrentEnergy) {
              SetQuenchingFactor(physicsEntry->fQuenchingArray->Value(CurrentEnergy));
            } else {  // otherwise interpolates QF
              slope = (physicsEntry->fQuenchingArray->Value(CurrentEnergy) -
                       physicsEntry->fQuenchingArray->Value(PreviousEnergy)) /
                      (CurrentEnergy - PreviousEnergy);
              SetQuenchingFactor(slope * (PrimEn - PreviousEnergy) +
                                 physicsEntry->fQuenchingArray->Value(PreviousEnergy));
            }
          }
        }
      } else {
        SetQuenchingFactor(1.0);  // Default to 1 if no table and no user quenching factor
      }
    }

    // set positions, directions, etc.
    G4StepPoint *pPreStepPoint = aStep.GetPreStepPoint();
    G4StepPoint *pPostStepPoint = aStep.GetPostStepPoint();

    G4ThreeVector x0 = pPreStepPoint->GetPosition();
    G4ThreeVector p0 = pPreStepPoint->GetMomentumDirection();
    G4double t0 = pPreStepPoint->GetGlobalTime();

    // Finally ready to start generating the event
    // figure out how many photons we want to make

    // Retrieve the total Light Yield for this material
    G4double ScintillationYield = physicsEntry->fLightYield;

    G4double num_comp = 0;
    G4MaterialPropertiesTable *mpt_scint = aMaterial->GetMaterialPropertiesTable();
    if (mpt_scint->ConstPropertyExists("NUM_COMP")) {
      num_comp = mpt_scint->GetConstProperty("NUM_COMP");
    }

    G4int numSecondaries = 0;
    G4double weight = 0;
    G4double p_reemission = 0;

    // Markers to record which component absorbed or emitted the photon
    int absorbed = 0;
    int emitted = -1;

    if (flagReemission) {
      // Reemission with a single component
      if (physicsEntry->fReemissionIntegral) {
        G4MaterialPropertyVector *mpv_scint_reemission = mpt_scint->GetProperty("REEMISSION_PROB");
        if (mpv_scint_reemission == nullptr) {
          goto PostStepDoIt_DONE;
        }
        p_reemission = mpv_scint_reemission->Value(aTrack.GetKineticEnergy());
      }
      // Reemission with multiple components: use relative absorption probabilities
      else if (num_comp) {
        const G4double tot = mpt_scint->GetProperty("ABSLENGTH")->Value(aTrack.GetKineticEnergy());
        const G4double rand = G4UniformRand();
        G4double prob = 0;
        for (int j = 0; j < num_comp; j++) {
          // calculate the probability that each component absorbs
          char temp[32];
          sprintf(temp, "ABSLENGTH%d", j);
          G4MaterialPropertyVector *mpv_absl = mpt_scint->GetProperty(temp);

          G4double absl = DBL_MAX;
          if (mpv_absl) {
            absl = mpv_absl->Value(aTrack.GetKineticEnergy());
            prob += tot / absl;

            if (rand <= prob) {
              // if the component absorbed the photon, get the reemission probability
              char temp2[32];
              sprintf(temp2, "REEMISSION_PROB%d", j);
              G4MaterialPropertyVector *mpv_scint_reemission = mpt_scint->GetProperty(temp2);
              if (mpv_scint_reemission == nullptr) {
                goto PostStepDoIt_DONE;
              }
              p_reemission = mpv_scint_reemission->Value(aTrack.GetKineticEnergy());
              absorbed = j;
              goto ENDLOOP;
            }
          }
        }
      }
    ENDLOOP:
      if (G4UniformRand() >= p_reemission) {
        goto PostStepDoIt_DONE;
      }
      numSecondaries = 1;  // Additional photons can be re-emitted below if REEMISSION_MULT == true
      weight = aTrack.GetWeight();
    }
    // apply Birks' law
    else {
      G4double birksConstant = physicsEntry->fBirksConstant;
      G4double QuenchedTotalEnergyDeposit = fQuenching->QuenchedEnergyDeposit(aStep, birksConstant);

      // track total edep, quenched edep
      fTotEdep += TotalEnergyDeposit;
      fTotEdepQuenched += QuenchedTotalEnergyDeposit;
      fTotEdepTime = t0;
      fScintCentroidSum += QuenchedTotalEnergyDeposit * (x0 + p0 * (0.5 * aStep.GetStepLength()));

      // now we are done if we are not actually making photons here
      if (!fDoScintillation) {
        goto PostStepDoIt_DONE;
      }

      // calculate MeanNumPhotons
      G4double MeanNumPhotons = ScintillationYield * GetQuenchingFactor() * QuenchedTotalEnergyDeposit *
                                (1.0 + birksConstant * (physicsEntry->fRefdEdx));

      if (MeanNumPhotons <= 0.0) {
        goto PostStepDoIt_DONE;
      }

      // randomize number of TRACKS (not photons)
      // this gets statistics right for number of PE after applying
      // boolean random choice to final absorbed track (change from
      // old method of applying binomial random choice to final absorbed
      // track, which did want poissonian number of photons divided
      // as evenly as possible into tracks)
      // Note for weight=1, there's no difference between tracks and photons.
      G4double MeanNumTracks = MeanNumPhotons / fMeanPhotonsPerSecondary / RAT::PhotonThinning::GetFactor();
      MeanNumTracks /= RAT::PhotonThinning::GetScintillationThinningFactor();

      G4double resolutionScale = physicsEntry->fResolutionScale;
      if (resolutionScale > 1.0) {
        MeanNumTracks =
            CLHEP::RandGauss::shoot(MeanNumTracks, sqrt(resolutionScale * resolutionScale - 1.0) * MeanNumTracks);
      }
      numSecondaries = (G4int)(CLHEP::RandPoisson::shoot(MeanNumTracks));

      weight = fMeanPhotonsPerSecondary;
      if (numSecondaries > fMaxTracksPerStep) {
        // it's probably better to just set meanPhotonsPerSecondary to
        // a big number if you want a small number of secondaries, but
        // this feature is retained for backwards compatibility.
        weight = weight * numSecondaries / fMaxTracksPerStep;
        numSecondaries = fMaxTracksPerStep;
      }
    }

    // if there are no photons, then we're all done now
    if (numSecondaries <= 0) {
      // return unchanged particle and no secondaries
      fAParticleChange.SetNumberOfSecondaries(0);
      goto PostStepDoIt_DONE;
    }

    // Okay, we will make at least one secondary.
    // Notify the proper authorities.
    fAParticleChange.SetNumberOfSecondaries(numSecondaries);
    if (!flagReemission) {
      if (aTrack.GetTrackStatus() == fAlive) {
        fAParticleChange.ProposeTrackStatus(fSuspend);
      }
    }

    // now look up waveform information we need to add the secondaries
    // reemission - single component material info
    G4PhysicsOrderedFreeVector *ReemissionIntegral = physicsEntry->fReemissionIntegral;
    G4PhysicsOrderedFreeVector *ReemitWaveformIntegral = physicsEntry->fReemissionTimeIntegral;
    // reemission - multi-component material info
    std::vector<G4PhysicsOrderedFreeVector *> ReemitVector = physicsEntry->fReemissionTimeVector;
    std::vector<G4PhysicsOrderedFreeVector *> ReemitSpectrumVector = physicsEntry->fReemissionSpectrumVector;
    // scintillation - single component material info
    G4PhysicsOrderedFreeVector *ScintillationIntegral = physicsEntry->fSpectrumIntegral;
    G4PhysicsOrderedFreeVector *WaveformIntegral = physicsEntry->fTimeIntegral;
    // scintillation - multi-component material info
    std::vector<G4PhysicsOrderedFreeVector *> EmitVector = physicsEntry->fEmissionTimeVector;
    std::vector<G4PhysicsOrderedFreeVector *> EmitSpectrumVector = physicsEntry->fEmissionSpectrumVector;
    std::vector<G4double> LightYieldVector = physicsEntry->fLightYieldVector;

#ifdef RATDEBUG
    if (num_comp && !ScintillationIntegral) {
      G4double LYtotal = 0;
      for (G4double LY : LightYieldVector) {
        LYtotal += LY;
      }  // CROSS CHECK
      if (LYtotal != ScintillationYield)
        RAT::warn << "GLG4Scint: LYtotal (" << LYtotal << " != ScintillationYield (" << ScintillationYield << ")."
                  << newline;
    }
#endif

    // if reemission, first determine photon energies to determine actual number of secondaries
    std::vector<G4double> reemitMomenta;
    if (flagReemission) {
      G4double CIIvalue;
      G4bool this_is_STUPID;
      G4double sampledMomentum;
      G4double sumReemitMomenta = 0;

      G4bool flagReemitMult = false;
      if (mpt_scint->ConstPropertyExists("REEMISSION_MULT")) {
        flagReemitMult = (mpt_scint->GetConstProperty("REEMISSION_MULT") > 0) ? true : false;
      }

      G4PhysicsOrderedFreeVector *ReemissionInt_temp = nullptr;
      if (ReemissionIntegral) {  // Get reemission spectrum for material
        ReemissionInt_temp = ReemissionIntegral;
      } else if (num_comp && ReemitSpectrumVector[absorbed]) {  // Get reemission spectrum for absorbing component
        ReemissionInt_temp = ReemitSpectrumVector[absorbed];
      }

      for (G4int n = 0; flagReemitMult || (!flagReemitMult && n < 1); n++) {
        CIIvalue = G4UniformRand() *
                   ReemissionInt_temp->GetValue(aTrack.GetKineticEnergy() - sumReemitMomenta, this_is_STUPID);
        if (CIIvalue == 0.0) {  // stop accumulating re-emitted photons
          numSecondaries = n;
          break;
        }
        sampledMomentum = ReemissionInt_temp->GetEnergy(CIIvalue);

#ifdef RATDEBUG
        if (!flagReemitMult && sampledMomentum > aTrack.GetKineticEnergy()) {
          RAT::warn << "GLG4Scint: Error: sampled reemitted photon momentum " << sampledMomentum
                    << " is greater than track energy " << aTrack.GetKineticEnergy() << newline;
        }
        if (fVerboseLevel > 1) {
          if (n == 0) RAT::debug << "GLG4Scint: orignalMomentum = " << aTrack.GetKineticEnergy() << newline;
          RAT::debug << "GLG4Scint: reemittedSampledMomentum = " << sampledMomentum << newline
                     << "GLG4Scint: reemittedCIIvalue =        " << CIIvalue << newline;
        }
#endif
        sumReemitMomenta += sampledMomentum;
        if (sumReemitMomenta > aTrack.GetKineticEnergy()) {  // Energy no longer conserved - exclude this photon
          sumReemitMomenta -= sampledMomentum;
          numSecondaries = n;
          break;
        }
        numSecondaries = n + 1;
        reemitMomenta.push_back(sampledMomentum);
      }
      fAParticleChange.ProposeLocalEnergyDeposit(aTrack.GetKineticEnergy() - sumReemitMomenta);
      fAParticleChange.SetNumberOfSecondaries(numSecondaries);
      if (numSecondaries == 0) goto PostStepDoIt_DONE;
    }

    /// Add the secondaries
    for (G4int iSecondary = 0; iSecondary < numSecondaries; iSecondary++) {
      // Determine photon momentum
      G4double sampledMomentum = 0;
      if (flagReemission) {  // reemission
        sampledMomentum = reemitMomenta[iSecondary];
      } else {  // scintillation
        G4double CIIvalue;
        if (ScintillationIntegral) {  // single component material
          CIIvalue = G4UniformRand() * ScintillationIntegral->GetMaxValue();
          sampledMomentum = ScintillationIntegral->GetEnergy(CIIvalue);
        } else if (num_comp) {  // emission from a component
          G4double LY, prob = 0;
          const G4double rand = G4UniformRand();
          for (int j = 0; j < num_comp; j++) {
            LY = LightYieldVector[j];         // light yield for component j
            prob += LY / ScintillationYield;  // sum relative emission probabilities
            if (rand <= prob) {
              emitted = j;
              break;
            }
          }
          // Get emission spectrum for component
          if (emitted >= 0) {
            CIIvalue = G4UniformRand() * EmitSpectrumVector[emitted]->GetMaxValue();
            sampledMomentum = EmitSpectrumVector[emitted]->GetEnergy(CIIvalue);
          } else {  // should not happen
            RAT::warn << "GLG4Scint: ERROR: Expected emission from component, but found none." << newline;
          }
        }
#ifdef RATVERBOSE
        RAT::debug << "GLG4Scint: sampledMomentum = " << sampledMomentum << newline;
        RAT::debug << "GLG4Scint: CIIvalue =        " << CIIvalue << newline;
#endif
      }

      // Check whether sampled wavelength is within the accepted range, and skip if not
      double wvl = (CLHEP::twopi * CLHEP::hbarc) / ((sampledMomentum * CLHEP::MeV) * CLHEP::nm);
      if (wvl < RAT::PhotonThinning::GetScintillationLowerWavelengthThreshold() ||
          wvl > RAT::PhotonThinning::GetScintillationUpperWavelengthThreshold()) {
        continue;
      }

      // Generate random photon direction
      G4double cost = 1. - 2. * G4UniformRand();
      G4double sint = sqrt(1. - cost * cost);

      G4double phi = CLHEP::twopi * G4UniformRand();
      G4double sinp = sin(phi);
      G4double cosp = cos(phi);

      G4double px = sint * cosp;
      G4double py = sint * sinp;
      G4double pz = cost;

      // Create photon momentum direction vector
      G4ParticleMomentum photonMomentum(px, py, pz);

      // Determine polarization of new photon
      G4double sx = cost * cosp;
      G4double sy = cost * sinp;
      G4double sz = -sint;

      G4ThreeVector photonPolarization(sx, sy, sz);

      G4ThreeVector perp = photonMomentum.cross(photonPolarization);

      phi = CLHEP::twopi * G4UniformRand();
      sinp = sin(phi);
      cosp = cos(phi);

      photonPolarization = cosp * photonPolarization + sinp * perp;

      photonPolarization = photonPolarization.unit();

      // Generate a new photon:
      G4DynamicParticle *aScintillationPhoton = new G4DynamicParticle(G4OpticalPhoton::OpticalPhoton(), photonMomentum);
      aScintillationPhoton->SetPolarization(photonPolarization.x(), photonPolarization.y(), photonPolarization.z());

      aScintillationPhoton->SetKineticEnergy(sampledMomentum);

      // Generate new G4Track object:
      G4ThreeVector aSecondaryPosition;
      G4double deltaTime;
      if (flagReemission) {
        deltaTime = pPostStepPoint->GetGlobalTime() - t0;
        aSecondaryPosition = pPostStepPoint->GetPosition();
      } else {
        G4double delta = G4UniformRand() * aStep.GetStepLength();
        aSecondaryPosition = x0 + delta * p0;

        // start deltaTime based on where on the track it happened
        deltaTime = delta / ((pPreStepPoint->GetVelocity() + pPostStepPoint->GetVelocity()) / 2.);
      }

      // delay for scintillation or re-emission time
      G4double sampledDelayTime = 0;
      // Re-emission timing
      if (flagReemission) {
        if (ReemitWaveformIntegral) {  // for a single component
          G4double WFvalue = G4UniformRand() * ReemitWaveformIntegral->GetMaxValue();
          sampledDelayTime = ReemitWaveformIntegral->GetEnergy(WFvalue);
        } else if (num_comp && (ReemitVector.size() > 0) && ReemitVector[absorbed]) {  // for the absorbing component
          G4double WFvalue = G4UniformRand() * ReemitVector[absorbed]->GetMaxValue();
          sampledDelayTime = ReemitVector[absorbed]->GetEnergy(WFvalue);
        }
      }
      // Scintillation timing
      else {
        if (WaveformIntegral) {  // for a single component
          G4double WFvalue = G4UniformRand() * WaveformIntegral->GetMaxValue();
          sampledDelayTime = WaveformIntegral->GetEnergy(WFvalue);
        } else if (num_comp && emitted >= 0 && EmitVector[emitted]) {  // for the emitting component
          G4double WFvalue = G4UniformRand() * EmitVector[emitted]->GetMaxValue();
          sampledDelayTime = EmitVector[emitted]->GetEnergy(WFvalue);
        }
      }
      deltaTime += sampledDelayTime;

      // set secondary time
      G4double aSecondaryTime = t0 + deltaTime;

      // create secondary track
      G4Track *aSecondaryTrack = new G4Track(aScintillationPhoton, aSecondaryTime, aSecondaryPosition);
      // Add the information to the track history (if it exists)
      RAT::TrackInfo *trackInfo = new RAT::TrackInfo();
      trackInfo->SetCreatorStep(aTrack.GetCurrentStepNumber());
      // Copy the history of the parent into this one
      if (aTrack.GetParticleDefinition() == G4OpticalPhoton::OpticalPhoton()) {
        // This new track is produced from an existing photon.
        // Copy the parent history into it
        // FIXME rat-pac does not have this functionality
        // trackHistory->ImportHistory(dynamic_cast<RAT::UserTrackInformation
        // *>(aTrack.GetUserInformation()));
      }
      aSecondaryTrack->SetWeight(weight);
      aSecondaryTrack->SetParentID(aTrack.GetTrackID());
      if (flagReemission) {
        if (ReemissionIntegral) {
          aSecondaryTrack->SetCreatorProcess(&fReemissionProcess);
          trackInfo->SetCreatorProcess(fReemissionProcess.GetProcessName());
        } else if (num_comp && (ReemitVector.size() > 0)) {
          // If multi-component, get absorbing component and assign
          // the creator process "Reemission_from_comp.."
          std::list<GLG4DummyProcess *>::iterator it;
          it = fReemissionProcessVector.begin();
          advance(it, absorbed);
          aSecondaryTrack->SetCreatorProcess((*it));
          trackInfo->SetCreatorProcess((*it)->GetProcessName());
        }
      } else {
        if (ScintillationIntegral) {
          aSecondaryTrack->SetCreatorProcess(&fScintProcess);
          trackInfo->SetCreatorProcess(fScintProcess.GetProcessName());
        } else if (num_comp && emitted >= 0) {
          // If multi-component, get emitting component and assign
          // the creator process "Emission_from_comp.."
          std::list<GLG4DummyProcess *>::iterator it;
          it = fEmissionProcessVector.begin();
          advance(it, emitted);
          aSecondaryTrack->SetCreatorProcess((*it));
          trackInfo->SetCreatorProcess((*it)->GetProcessName());
        }
      }

      aSecondaryTrack->SetUserInformation(trackInfo);
      // add the secondary to the ParticleChange object
      fAParticleChange.SetSecondaryWeightByProcess(true);  // recommended
      fAParticleChange.AddSecondary(aSecondaryTrack);

      aSecondaryTrack->SetWeight(weight);
      // The above line is necessary because AddSecondary() overrides
      // our setting of the secondary track weight, in Geant4.3.1 & earlier.
      // (and also later, at least until Geant4.7 (and beyond?)
      //  -- maybe not required if SetWeightByProcess(true) called,
      //  but we do both, just to be sure)
    }
    // done iSecondary loop
  }
PostStepDoIt_DONE:
  if (!flagReemission) {
    fsScintillatedCount += fAParticleChange.GetNumberOfSecondaries();
  } else {
    fsReemittedCount += fAParticleChange.GetNumberOfSecondaries();
  }
#ifdef RATDEBUG
  timer.Stop();
  GLG4Scint_tottime += timer.GetUserElapsed();
  GLG4Scint_num_phots += fAParticleChange.GetNumberOfSecondaries();
#endif
#ifdef RATVERBOSE
  if (fVerboseLevel > 1) {
    RAT::detail << "\n Exiting from GLG4Scint::DoIt -- NumberOfSecondaries = "
                << fAParticleChange.GetNumberOfSecondaries() << newline;
  }
#endif

  return &fAParticleChange;
}

////////////////////////////////////////////////////////////////
// the generic (static) PostPostStepDoIt
G4VParticleChange *GLG4Scint::GenericPostPostStepDoIt(const G4Step *pStep) {
  G4Track *track = pStep->GetTrack();
  G4double mass = track->GetDynamicParticle()->GetMass();
  std::vector<GLG4Scint *>::iterator it = fMasterVectorOfGLG4Scint.begin();
  for (int i = fMasterVectorOfGLG4Scint.size(); (i--) > 1;) {
    it++;
    if (mass < (*it)->fLowerMassLimit) {
      return (*(--it))->PostPostStepDoIt(*track, *pStep);
    }
  }
  return (*it)->PostPostStepDoIt(*track, *pStep);
}

////////////////////////////////////////////////////////////////
// build physics tables for the scintillation process
// --------------------------------------------------
//

static G4PhysicsOrderedFreeVector *Integrate_MPV_to_POFV(G4MaterialPropertyVector *inputVector) {
  G4PhysicsOrderedFreeVector *aPhysicsOrderedFreeVector = new G4PhysicsOrderedFreeVector();

  // Retrieve the first intensity point in vector
  // of (photon momentum, intensity) pairs

  unsigned int i = 0;
  G4double currentIN = (*inputVector)[i];

  if (currentIN >= 0.0) {
    // Create first (photon momentum, Scintillation
    // Integral pair

    G4double currentPM = inputVector->Energy(i);

    G4double currentCII = 0.0;

    aPhysicsOrderedFreeVector->InsertValues(currentPM, currentCII);

    // Set previous values to current ones prior to loop
    G4double prevPM = currentPM;
    G4double prevCII = currentCII;
    G4double prevIN = currentIN;

    // loop over all (photon momentum, intensity)
    // pairs stored for this material
    while (i < inputVector->GetVectorLength() - 1) {
      i++;
      currentPM = inputVector->Energy(i);
      currentIN = (*inputVector)[i];

      currentCII = 0.5 * (prevIN + currentIN);

      currentCII = prevCII + (currentPM - prevPM) * currentCII;

      aPhysicsOrderedFreeVector->InsertValues(currentPM, currentCII);

      prevPM = currentPM;
      prevCII = currentCII;
      prevIN = currentIN;
    }
  }

  return aPhysicsOrderedFreeVector;
}

////////////////////////////////////////////////////////////////
// MyPhysicsTable (nested class) definitions
////////////////////////////////////////////////////////////////

////////////////
// "static" members of the class
// [N.B. don't use "static" keyword here, because it means something
//  entirely different in this context.]
////////////////

GLG4Scint::MyPhysicsTable *GLG4Scint::MyPhysicsTable::fHead = nullptr;

////////////////
// constructor
////////////////

GLG4Scint::MyPhysicsTable::MyPhysicsTable() {
  fName = 0;
  fNext = 0;
  fUsedByCount = 0;
  fData = 0;
  fLength = 0;
}

////////////////
// destructor
////////////////

GLG4Scint::MyPhysicsTable::~MyPhysicsTable() {
  if (fUsedByCount != 0) {
    RAT::warn << "GLG4Scint: Error, GLG4Scint::MyPhysicsTable is being deleted with used_by_count=" << fUsedByCount
              << newline;
    return;
  }
  if (fName) {
    delete fName;
  }
  if (fData) {
    delete[] fData;
  }
}

////////////////
// member functions
////////////////

void GLG4Scint::MyPhysicsTable::Dump(void) const {
  RAT::info << " GLG4Scint::MyPhysicsTable {" << newline << "  fName=" << (*fName) << newline << "  fLength=" << fLength
            << newline << "  fUsedByCount=" << fUsedByCount << newline;
  for (G4int i = 0; i < fLength; i++) {
    RAT::info << "  data[" << i << "]= { // " << (*G4Material::GetMaterialTable())[i]->GetName() << newline;
    RAT::info << "   spectrumIntegral=";
    if (fData[i].fSpectrumIntegral) {
      (fData[i].fSpectrumIntegral)->DumpValues();
    } else {
      RAT::info << "nullptr" << newline;
    }

    RAT::info << "   reemissionIntegral=";
    if (fData[i].fReemissionIntegral) {
      (fData[i].fReemissionIntegral)->DumpValues();
    } else {
      RAT::info << "nullptr" << newline;
    }

    RAT::info << "   timeIntegral=";
    if (fData[i].fTimeIntegral) {
      (fData[i].fTimeIntegral)->DumpValues();
    } else {
      RAT::info << "nullptr" << newline;
    }

    RAT::info << "   resolutionScale=" << fData[i].fResolutionScale << "   birksConstant=" << fData[i].fBirksConstant
              << "   ref_dE_dx=" << fData[i].fRefdEdx << newline << "   light yield=" << fData[i].fLightYield
              << newline;

    RAT::info << "Quenching = ";
    if (fData[i].fQuenchingArray != nullptr) {
      fData[i].fQuenchingArray->DumpValues();
    } else {
      RAT::info << "nullptr" << newline << "  }" << newline;
    }
  }
  RAT::info << " }" << newline;
}

GLG4Scint::MyPhysicsTable *GLG4Scint::MyPhysicsTable::FindOrBuild(const G4String &name) {
  // head should always exist and should always be the default (name=="")
  if (fHead == nullptr) {
    fHead = new MyPhysicsTable;
    fHead->Build("");
  }

  MyPhysicsTable *rover = fHead;
  while (rover) {
    if (name == *(rover->fName)) {
      return rover;
    }
    rover = rover->fNext;
  }

  rover = new MyPhysicsTable;
  rover->Build(name);
  rover->fNext = fHead->fNext;  // always keep head pointing to default
  fHead->fNext = rover;

  return rover;
}

void GLG4Scint::MyPhysicsTable::Build(const G4String &newname) {
  if (fName) {
    delete fName;
  }
  if (fData) {
    delete[] fData;
  }

  fName = new G4String(newname);

  const G4MaterialTable *theMaterialTable = G4Material::GetMaterialTable();
  fLength = G4Material::GetNumberOfMaterials();

  fData = new Entry[fLength];

  // create new physics tables

  for (G4int i = 0; i < fLength; i++) {
    // look for material properties table entry.
    const G4Material *aMaterial = (*theMaterialTable)[i];
    const G4String &matName = aMaterial->GetName();

    // ask data[i] to Build itself
    fData[i].Build(*fName, matName, i, aMaterial->GetMaterialPropertiesTable());
  }
}

////////////////
// constructor for Entry
////////////////

GLG4Scint::MyPhysicsTable::Entry::Entry() {
  fSpectrumIntegral = nullptr;
  fReemissionIntegral = nullptr;
  fTimeIntegral = nullptr;
  fReemissionTimeIntegral = nullptr;
  fOwnSpectrumIntegral = 0;
  fOwnTimeIntegral = 0;
  fOwnReemissionTimeIntegral = 0;
  fResolutionScale = 1.0;
  fLightYield = 0.0;
  fBirksConstant = fRefdEdx = 0.0;
}

////////////////
// destructor for Entry
////////////////

GLG4Scint::MyPhysicsTable::Entry::~Entry() {
  if (fSpectrumIntegral && fOwnSpectrumIntegral) {
    delete fSpectrumIntegral;
    delete fReemissionIntegral;
  }
  if (fTimeIntegral && fOwnTimeIntegral) {
    delete fTimeIntegral;
  }
  if (fReemissionTimeIntegral && fOwnReemissionTimeIntegral) {
    delete fReemissionTimeIntegral;
  }
}

////////////////
// Build for Entry
////////////////

void GLG4Scint::MyPhysicsTable::Entry::Build(const G4String &name, const G4String &matName, int material_index,
                                             G4MaterialPropertiesTable *aMaterialPropertiesTable) {
  // delete old data, if any
  if (fSpectrumIntegral && fOwnSpectrumIntegral) {
    delete fSpectrumIntegral;
    delete fReemissionIntegral;
  }
  if (fTimeIntegral && fOwnTimeIntegral) {
    delete fTimeIntegral;
  }
  if (fReemissionTimeIntegral && fOwnReemissionTimeIntegral) {
    delete fReemissionTimeIntegral;
  }

  // set defaults
  fSpectrumIntegral = nullptr;
  fReemissionIntegral = nullptr;
  fTimeIntegral = nullptr;
  fReemissionTimeIntegral = nullptr;
  fResolutionScale = 1.0;
  fBirksConstant = fRefdEdx = 0.0;
  fLightYield = 0.0;
  fQuenchingArray = nullptr;

  // exit, leaving default values, if no material properties
  if (!aMaterialPropertiesTable) {
    return;
  }

  /// Retrieve vector of scintillation wavelength intensity for material from
  /// the material's optical properties table ("SCINTILLATION")

  std::stringstream property_string;
  property_string.str("");
  property_string << "SCINTILLATION" << name;
  G4MaterialPropertyVector *theScintillationLightVector =
      aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());
  // save the spectrum integral
  if (theScintillationLightVector) {
    fSpectrumIntegral = Integrate_MPV_to_POFV(theScintillationLightVector);
    fOwnSpectrumIntegral = 1;
  } else {
    fOwnSpectrumIntegral = 0;
  }

  property_string.str("");
  property_string << "SCINTILLATION_WLS" << name;
  G4MaterialPropertyVector *theReemissionLightVector =
      aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());
  // save the spectrum integral
  if (theReemissionLightVector) {
    fReemissionIntegral = Integrate_MPV_to_POFV(theReemissionLightVector);
  }

  /// Get the emission and re-emission spectra of each component, if they exist
  G4double num_comp = 0;
  if (aMaterialPropertiesTable->ConstPropertyExists("NUM_COMP")) {
    num_comp = aMaterialPropertiesTable->GetConstProperty("NUM_COMP");
    if (num_comp <= 0) {
      RAT::Log::Die("Cannot set NUM_COMP < 1.");
    }
  }

  if (num_comp) {
    for (int cnt = 0; cnt < num_comp; cnt++) {
      // Scintillation
      property_string.str("");
      property_string << "SCINTILLATION" << cnt << name;
      G4MaterialPropertyVector *emissionVector = aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());

      if (emissionVector) {
        if (theScintillationLightVector) {
          RAT::Log::Die("Cannot define both a material and a component SCINTILLATION spectrum.");
        }

        G4PhysicsOrderedFreeVector *tempVector = Integrate_MPV_to_POFV(emissionVector);
        fEmissionSpectrumVector.push_back(tempVector);
      } else {
        fEmissionSpectrumVector.push_back(nullptr);
      }

      // Re-emission
      property_string.str("");
      property_string << "SCINTILLATION_WLS" << cnt << name;
      G4MaterialPropertyVector *reemissionVector =
          aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());

      if (reemissionVector) {
        if (theReemissionLightVector) {
          RAT::Log::Die("Cannot define both a material and a component SCINTILLATION_WLS spectrum.");
        }
        // Also enforce requirement of component absorption lengths
        char temp[32];
        sprintf(temp, "ABSLENGTH%d", cnt);
        G4MaterialPropertyVector *mpv_absl = aMaterialPropertiesTable->GetProperty(temp);
        if (!mpv_absl) {
          RAT::Log::Die("Should not define a SCINTILLATION_WLS spectrum for a component that has no ABSLENGTH.");
        }

        G4PhysicsOrderedFreeVector *tempVector = Integrate_MPV_to_POFV(reemissionVector);
        fReemissionSpectrumVector.push_back(tempVector);
      } else {
        fReemissionSpectrumVector.push_back(nullptr);
      }
    }
  }

  /// Retrieve value(s) of scintillation light yield for material from
  /// the material's optical properties field ("LIGHT_YIELD")

  if (theScintillationLightVector) {  // if single component emission
    if (aMaterialPropertiesTable->ConstPropertyExists("LIGHT_YIELD")) {
      fLightYield = aMaterialPropertiesTable->GetConstProperty("LIGHT_YIELD");
    } else {
      fLightYield = fSpectrumIntegral->GetMaxValue();
      RAT::warn << "GLG4Scint: No light yield parameter for " << matName << " for " << (name == "" ? "default" : name)
                << " scint process, assuming it is implicit in the scintillation integral" << newline;
    }
  } else if (num_comp) {  // if multi-component information
    G4double LY;
    G4double LYtot = 0;
    for (int cnt = 0; cnt < num_comp; cnt++) {
      LY = 0;
      property_string.str("");
      property_string << "LIGHT_YIELD" << cnt << name;
      if (fEmissionSpectrumVector[cnt]) {
        if (theScintillationLightVector) {
          RAT::Log::Die("Cannot define a component LIGHT_YIELD while defining a material SCINTILLATION spectrum.");
        }
        if (aMaterialPropertiesTable->ConstPropertyExists((property_string.str()).c_str())) {
          LY = aMaterialPropertiesTable->GetConstProperty((property_string.str()).c_str());
        } else {
          LY = fEmissionSpectrumVector[cnt]->GetMaxValue();
        }
      }
      LYtot += LY;
      fLightYieldVector.push_back(LY);
    }
    // Overwrite total light yield with sum of components only if components will be used
    if (!theScintillationLightVector) {
      fLightYield = LYtot;
    }
  }

  /// Retrieve vector of scintillation time profile for the material from
  /// the material's optical properties table ("SCINTWAVEFORM")

  if (theScintillationLightVector) {  // if single component emission
    property_string.str("");
    property_string << "SCINTWAVEFORM" << name;
    G4MaterialPropertyVector *theWaveForm = aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());

    G4double rise_time = 0.0;
    property_string.str("");
    property_string << "SCINT_RISE_TIME" << name;
    if (aMaterialPropertiesTable->ConstPropertyExists(property_string.str().c_str())) {
      rise_time = aMaterialPropertiesTable->GetConstProperty(property_string.str().c_str());
    } else if (aMaterialPropertiesTable->ConstPropertyExists("SCINT_RISE_TIME")) {
      rise_time = aMaterialPropertiesTable->GetConstProperty("SCINT_RISE_TIME");
    }
    RAT::Log::Assert(rise_time >= 0.0,
                     "GLG4Scint::MyPhysicsTable::Entry::Build(): rise time must be greater than or equal to 0.");

    if (theWaveForm) {
      // do we have time-series or decay-time data?
      if (theWaveForm->GetEnergy(0) >= 0.0) {
        // we have digitized waveform (time-series) data
        // find the integral
        fTimeIntegral = Integrate_MPV_to_POFV(theWaveForm);
        fOwnTimeIntegral = 1;
      } else {
        // we have decay-time data.
        // sanity-check user's values:
        // issue a warning if they are nonsense, but continue
        if (theWaveForm->Energy(theWaveForm->GetVectorLength() - 1) > 0.0) {
          RAT::warn << "GLG4Scint::MyPhysicsTable::Entry::Build():  "
                    << "SCINTWAVEFORM" << name << " for material " << matName
                    << " has both positive and negative X values.  "
                    << " Undefined results will ensue!" << newline;
        }

        // Set the bin width to 100 times smaller than the smallest decay constant.
        G4double mintime = -1.0 * (theWaveForm->GetMaxEnergy());
        G4double bin_width = mintime / 100;

        // Set the maximum time for the PDF to 30 times the longest decay constant.
        G4double maxtime = -30.0 * (theWaveForm->GetEnergy(0));
        int nbins = ((int)(maxtime / bin_width)) + 1;

        G4double *tval = new G4double[nbins];
        G4double *ival = new G4double[nbins];

        // Set the time array, and zero out the CDF.
        for (int i = 0; i < nbins; i++) {
          tval[i] = i * maxtime / nbins;
          ival[i] = 0.0;
        }

        G4double ampl, decy;
        for (size_t j = 0; j < theWaveForm->GetVectorLength(); j++) {
          ampl = (*theWaveForm)[j];
          decy = -theWaveForm->Energy(j);
          for (int i = 0; i < nbins; i++) {
            if (rise_time != 0.0) {
              ival[i] += ampl * (decy * (1.0 - exp(-tval[i] / decy)) + rise_time * (exp(-tval[i] / rise_time) - 1)) /
                         (decy - rise_time);
            } else {
              ival[i] += ampl * (1.0 - exp(-tval[i] / decy));
            }
          }
        }

        // Divide the CDF by the value at the end of the array to make sure it is normalized to 1.
        for (int i = 0; i < nbins; i++) {
          ival[i] /= ival[nbins - 1];
        }

        fTimeIntegral = new G4PhysicsOrderedFreeVector(tval, ival, nbins);
        fOwnTimeIntegral = 1;

        // in Geant4.0.0, G4PhysicsOrderedFreeVector makes its own copy
        // of any array passed to its constructor, so ...
        delete[] tval;
        delete[] ival;
      }
    } else {
      fTimeIntegral = nullptr;
      fOwnTimeIntegral = 0;
    }
  }
  /// Get the emission time of each component, if it exists
  else if (num_comp) {
    for (int cnt = 0; cnt < num_comp; cnt++) {
      property_string.str("");
      property_string << "SCINTWAVEFORM" << name << cnt;
      G4MaterialPropertyVector *theWaveFormC = aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());

      G4double rise_time = 0.0;
      property_string.str("");
      property_string << "SCINT_RISE_TIME" << name;
      std::stringstream property_stringC;
      property_stringC.str("");
      property_stringC << "SCINT_RISE_TIME" << name << cnt;
      if (aMaterialPropertiesTable->ConstPropertyExists(property_stringC.str().c_str())) {
        rise_time = aMaterialPropertiesTable->GetConstProperty(property_stringC.str().c_str());
      } else if (aMaterialPropertiesTable->ConstPropertyExists(property_string.str().c_str())) {
        rise_time = aMaterialPropertiesTable->GetConstProperty(property_string.str().c_str());
      } else if (aMaterialPropertiesTable->ConstPropertyExists("SCINT_RISE_TIME")) {
        rise_time = aMaterialPropertiesTable->GetConstProperty("SCINT_RISE_TIME");
      }
      RAT::Log::Assert(rise_time >= 0.0,
                       "GLG4Scint::MyPhysicsTable::Entry::Build(): rise time must be greater than or equal to 0.");

      if (theWaveFormC) {
        // do we have time-series or decay-time data?
        if (theWaveFormC->GetEnergy(0) >= 0.0) {
          // we have digitized waveform (time-series) data
          // find the integral
          fEmissionTimeVector.push_back(Integrate_MPV_to_POFV(theWaveFormC));
        } else {
          // we have decay-time data.
          // sanity-check user's values:
          // issue a warning if they are nonsense, but continue
          if (theWaveFormC->GetMaxEnergy() > 0.0) {
            RAT::warn << "GLG4Scint::MyPhysicsTable::Entry::Build():  "
                      << "SCINTWAVEFORM" << name << cnt << " for material " << matName
                      << " has both positive and negative X values.  "
                      << " Undefined results will ensue!" << newline;
          }

          G4double maxtime = -30.0 * (theWaveFormC->GetEnergy(0));
          G4double mintime = -1.0 * (theWaveFormC->GetMaxEnergy());
          G4double bin_width = mintime / 100;
          int nbins = ((int)(maxtime / bin_width)) + 1;
          G4double *tval = new G4double[nbins];
          G4double *ival = new G4double[nbins];
          for (int i = 0; i < nbins; i++) {
            tval[i] = i * maxtime / nbins;
            ival[i] = 0.0;
          }

          G4double ampl, decy;
          for (size_t j = 0; j < theWaveFormC->GetVectorLength(); j++) {
            ampl = (*theWaveFormC)[j];
            decy = -theWaveFormC->Energy(j);
            for (int i = 0; i < nbins; i++) {
              if (rise_time != 0.0) {
                ival[i] += ampl * (decy * (1.0 - exp(-tval[i] / decy)) + rise_time * (exp(-tval[i] / rise_time) - 1)) /
                           (decy - rise_time);
              } else {
                ival[i] += ampl * (1.0 - exp(-tval[i] / decy));
              }
            }
          }

          for (int i = 0; i < nbins; i++) {
            ival[i] /= ival[nbins - 1];
          }

          G4PhysicsOrderedFreeVector *timeIntegralC = new G4PhysicsOrderedFreeVector(tval, ival, nbins);
          fEmissionTimeVector.push_back(timeIntegralC);

          // in Geant4.0.0, G4PhysicsOrderedFreeVector makes its own copy
          // of any array passed to its constructor, so ...
          delete[] tval;
          delete[] ival;
        }
      } else {
        fEmissionTimeVector.push_back(nullptr);
      }
    }
  }

  /// Retrieve vector of re-emission time profile for the material from
  /// the material's optical properties table ("REEMITWAVEFORM")

  if (theReemissionLightVector) {  // if single component re-emission
    G4MaterialPropertyVector *theReemitWaveForm = aMaterialPropertiesTable->GetProperty("REEMITWAVEFORM");

    if (theReemitWaveForm) {
      // do we have time-series or decay-time data?
      if (theReemitWaveForm->GetEnergy(0) >= 0.0) {
        // we have digitized waveform (time-series) data
        // find the integral
        fReemissionTimeIntegral = Integrate_MPV_to_POFV(theReemitWaveForm);
        fOwnReemissionTimeIntegral = 1;
      } else {
        // we have decay-time data.
        // sanity-check user's values:
        // issue a warning if they are nonsense, but continue
        if (theReemitWaveForm->GetMaxEnergy() > 0.0) {
          RAT::warn << "GLG4Scint::MyPhysicsTable::Entry::Build():  "
                    << "REEMITWAVEFORM for material " << matName
                    << " has both positive and negative X values.  "
                       " Undefined results will ensue!"
                    << newline;
        }

        G4double maxtime = -30.0 * (theReemitWaveForm->GetEnergy(0));
        G4double mintime = -1.0 * (theReemitWaveForm->GetMaxEnergy());
        G4double bin_width = mintime / 100;
        int nbins = ((int)(maxtime / bin_width)) + 1;
        G4double *tval = new G4double[nbins];
        G4double *ival = new G4double[nbins];
        for (int ii = 0; ii < nbins; ii++) {
          tval[ii] = ii * maxtime / nbins;
          ival[ii] = 0.0;
        }

        G4double ampl, decy;
        for (size_t j = 0; j < theReemitWaveForm->GetVectorLength(); j++) {
          ampl = (*theReemitWaveForm)[j];
          decy = theReemitWaveForm->Energy(j);
          for (int ii = 0; ii < nbins; ii++) {
            ival[ii] += ampl * (1.0 - exp(tval[ii] / decy));
          }
        }

        for (int ii = 0; ii < nbins; ii++) {
          ival[ii] /= ival[nbins - 1];
        }

        fReemissionTimeIntegral = new G4PhysicsOrderedFreeVector(tval, ival, nbins);
        fOwnReemissionTimeIntegral = 1;

        // in Geant4.0.0, G4PhysicsOrderedFreeVector makes its own copy
        // of any array passed to its constructor, so ...
        delete[] tval;
        delete[] ival;
      }
    } else {
      fReemissionTimeIntegral = nullptr;
      fOwnReemissionTimeIntegral = 0;
    }
  }
  /// Get the re-emission time of each component, if it exists
  else if (num_comp) {
    for (int cnt = 0; cnt < num_comp; cnt++) {
      property_string.str("");
      property_string << "REEMITWAVEFORM" << cnt;
      G4MaterialPropertyVector *theReemitWaveForm =
          aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());

      if (theReemitWaveForm) {
        // do we have time-series or decay-time data?
        if (theReemitWaveForm->GetEnergy(0) >= 0.0) {
          // we have digitized waveform (time-series) data
          // find the integral
          fReemissionTimeVector.push_back(Integrate_MPV_to_POFV(theReemitWaveForm));
        } else {
          // we have decay-time data.
          // sanity-check user's values:
          // issue a warning if they are nonsense, but continue
          if (theReemitWaveForm->GetMaxEnergy() > 0.0) {
            RAT::warn << "GLG4Scint::MyPhysicsTable::Entry::Build():  "
                      << "REEMITWAVEFORM" << cnt << " for material " << matName
                      << " has both positive and negative X values.  "
                      << " Undefined results will ensue!" << newline;
          }

          G4double maxtime = -30.0 * (theReemitWaveForm->GetEnergy(0));
          G4double mintime = -1.0 * (theReemitWaveForm->GetMaxEnergy());
          G4double bin_width = mintime / 100;
          int nbins = ((int)(maxtime / bin_width)) + 1;
          G4double *tval = new G4double[nbins];
          G4double *ival = new G4double[nbins];
          for (int ii = 0; ii < nbins; ii++) {
            tval[ii] = ii * maxtime / nbins;
            ival[ii] = 0.0;
          }

          G4double ampl, decy;
          for (size_t j = 0; j < theReemitWaveForm->GetVectorLength(); j++) {
            ampl = (*theReemitWaveForm)[j];
            decy = theReemitWaveForm->Energy(j);
            for (int ii = 0; ii < nbins; ii++) {
              ival[ii] += ampl * (1.0 - exp(tval[ii] / decy));
            }
          }

          for (int ii = 0; ii < nbins; ii++) {
            ival[ii] /= ival[nbins - 1];
          }

          fReemissionTimeIntegral = new G4PhysicsOrderedFreeVector(tval, ival, nbins);
          fReemissionTimeVector.push_back(fReemissionTimeIntegral);

          // in Geant4.0.0, G4PhysicsOrderedFreeVector makes its own copy
          // of any array passed to its constructor, so ...
          delete[] tval;
          delete[] ival;
        }
      } else {
        fReemissionTimeVector.push_back(nullptr);
      }
    }
  }

  /// Retrieve vector of scintillation "modifications" for material from
  /// the material's optical properties table ("SCINTMOD")

  property_string.str("");
  property_string << "SCINTMOD" << name;
  G4MaterialPropertyVector *theScintModVector = aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());
  if (theScintModVector == nullptr) {
    // use default if not particle-specific value given
    theScintModVector = aMaterialPropertiesTable->GetProperty("SCINTMOD");
  }

  if (theScintModVector) {
    // parse the entries in ScintMod
    //  ResolutionScale= ScintMod(0);
    //  BirksConstant= ScintMod(1);
    //  Ref_dE_dx= ScintMod(2);

    for (size_t jscint = 0; jscint < theScintModVector->GetVectorLength(); jscint++) {
      G4double key = theScintModVector->Energy(jscint);
      G4double value = (*theScintModVector)[jscint];

      if (key == 0.0) {
        fResolutionScale = value;
      } else if (key == 1.0) {
        fBirksConstant = value;
      } else if (key == 2.0) {
        fRefdEdx = value;
      } else {
        RAT::warn << "GLG4Scint::MyPhysicsTable::Entry::Build:  Warning, unknown key " << key << "in SCINTMOD" << name
                  << " for material " << matName << newline;
      }
    }
  }

  property_string.str("");
  property_string << "QF" << name;
  fQuenchingArray = aMaterialPropertiesTable->GetProperty((property_string.str()).c_str());
  // if (fQuenchingArray!=nullptr)
  //    fQuenchingArray->DumpVector();

  // Create dummy processes describing emission and reemission for each component
  // up to the max number of components in any material in RAT
  if (fEmissionProcessVector.size() < num_comp) {
    for (int i = fEmissionProcessVector.size(); i < num_comp; i++) {
      std::stringstream process_name;
      process_name << "EmissionFromComp" << i;
      GLG4DummyProcess *EmissionCompProcess = new GLG4DummyProcess((process_name.str()).c_str(), fUserDefined);
      fEmissionProcessVector.push_back(EmissionCompProcess);
    }
  }
  if (fReemissionProcessVector.size() < num_comp) {
    for (int i = fReemissionProcessVector.size(); i < num_comp; i++) {
      std::stringstream process_name;
      process_name << "ReemissionFromComp" << i;
      GLG4DummyProcess *ReemissionCompProcess = new GLG4DummyProcess((process_name.str()).c_str(), fUserDefined);
      fReemissionProcessVector.push_back(ReemissionCompProcess);
    }
  }
}

void GLG4Scint::SetNewValue(G4UIcommand *command, G4String newValues) {
  G4String commandName = command->GetCommandName();
  if (commandName == "on") {
    fDoScintillation = true;
  } else if (commandName == "off") {
    fDoScintillation = false;
  } else if (commandName == "reemission") {
    char *endptr;
    G4int i = strtol((const char *)newValues, &endptr, 0);
    if (*endptr != '\0') {  // non-numerical argument
      if (!(i = strcmp((const char *)newValues, "on"))) {
        fDoReemission = true;
      } else if (!(i = strcmp((const char *)newValues, "off"))) {
        fDoReemission = false;
      } else {
        RAT::warn << "Command /glg4scint/reemission given unknown parameter " << '\"' << newValues << '\"' << newline
                  << "  old value unchanged: " << (fDoReemission ? "on" : "off") << newline;
      }
    } else {
      fDoReemission = (i != 0);
    }
  } else if (commandName == "maxTracksPerStep") {
    G4int i = strtol((const char *)newValues, nullptr, 0);
    if (i > 0) {
      fMaxTracksPerStep = i;
    } else {
      RAT::warn << "Value must be greater than 0, old value unchanged" << newline;
    }
  } else if (commandName == "meanPhotonsPerSecondary") {
    G4double d = strtod((const char *)newValues, nullptr);
    if (d >= 1.0) {
      fMeanPhotonsPerSecondary = d;
    } else {
      RAT::warn << "Value must be >= 1.0, old value unchanged" << newline;
    }
  } else if (commandName == "verbose") {
    fVerboseLevel = strtol((const char *)newValues, nullptr, 0);
  } else if (commandName == "dump") {
    std::vector<GLG4Scint *>::iterator it = fMasterVectorOfGLG4Scint.begin();
    for (; it != fMasterVectorOfGLG4Scint.end(); it++) {
      (*it)->DumpInfo();
    }
  } else if (commandName == "setQF") {
    G4double d = strtod((const char *)newValues, nullptr);
    if (d <= 1.0) {
      SetQuenchingFactor(d);
      fUserQF = true;
    } else {
      RAT::warn << "The quenching factor is <= 1.0, old value unchanged" << newline;
    }
  } else {
    RAT::warn << "No GLG4Scint command named " << commandName << newline;
  }
  return;
}

G4String GLG4Scint::GetCurrentValue(G4UIcommand *command) {
  G4String commandName = command->GetCommandName();

  if (commandName == "on" || commandName == "off") {
    return fDoScintillation ? "on" : "off";
  } else if (commandName == "reemission") {
    return fDoReemission ? "1" : "0";
  } else if (commandName == "maxTracksPerStep") {
    char outbuff[64];
    sprintf(outbuff, "%d", fMaxTracksPerStep);
    return G4String(outbuff);
  } else if (commandName == "meanPhotonsPerSecondary") {
    char outbuff[64];
    sprintf(outbuff, "%g", fMeanPhotonsPerSecondary);
    return G4String(outbuff);
  } else if (commandName == "verbose") {
    char outbuff[64];
    sprintf(outbuff, "%d", fVerboseLevel);
    return G4String(outbuff);
  } else if (commandName == "dump") {
    return "?/glg4scint/dump not supported";
  } else if (commandName == "setQF") {
    char outbuff[64];
    sprintf(outbuff, "%g", GetQuenchingFactor());
    return G4String(outbuff);
  } else {
    return (commandName + " is not a valid GLG4Scint command");
  }
}
