////////////////////////////////////////////////////////////////////
#ifndef GLG4Scint_h
#define GLG4Scint_h 1
/*
    Declares GLG4Scint class and helpers.

    This file is part of the GenericLAND software library.

    Author: Glenn Horton-Smith (Tohoku) 28-Jan-1999

    Revision History:
      13 Nov 2014: Matt Strait - Fixed shadowed variable warning
      13 May 2015: W. Heintzelman - Modify ResetPhotonCount to allow setting to
                   non-zero values; correct error in GetScintillatedCount.
      29 Feb 2016: W Heintzelman - Add functions: GetTotEdepall, SetTotEdep
      23 Oct 2016: N Barros - Added UserTrackInformation objects to secondary
   particles to track the process history of the photons.
*/

// [see detailed class documentation below]

/////////////
// Includes
/////////////

#include <RAT/BirksLaw.hh>
#include <RAT/Log.hh>
#include <RAT/Quadrature.hh>
#include <RAT/QuenchingCalculator.hh>

#include "G4DynamicParticle.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalPhoton.hh"
#include "G4ParticleChange.hh"
#include "G4ParticleMomentum.hh"
#include "G4PhysicsOrderedFreeVector.hh"
#include "G4PhysicsTable.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4UImessenger.hh"
#include "G4VProcess.hh"
#include "globals.hh"
#include "list"
#include "local_g4compat.hh"
#include "templates.hh"
#include "vector"

// Dummy classes used as placeholders in new opticalphoton tracks so
// that G4Track users can figure out the name of the process which
// created the track.
class GLG4DummyProcess : public G4VProcess {
 public:
  GLG4DummyProcess(const G4String &aName = "NoName", G4ProcessType aType = fNotDefined) : G4VProcess(aName, aType){};

  // Bogus, not a real process
  virtual G4double AlongStepGetPhysicalInteractionLength(const G4Track & /*track*/, G4double /*previousStepSize*/,
                                                         G4double /*currentMinimumStep*/, G4double & /*proposedSafety*/,
                                                         G4GPILSelection * /*selection*/) {
    return 0;
  };

  virtual G4double AtRestGetPhysicalInteractionLength(const G4Track & /*track*/, G4ForceCondition * /*condition*/) {
    return 0;
  };

  virtual G4double PostStepGetPhysicalInteractionLength(const G4Track & /*track*/, G4double /*previousStepSize*/,
                                                        G4ForceCondition * /*condition*/) {
    return 0;
  };

  virtual G4VParticleChange *PostStepDoIt(const G4Track & /*track*/, const G4Step & /*stepData*/) { return 0; };

  virtual G4VParticleChange *AlongStepDoIt(const G4Track & /*track*/, const G4Step & /*stepData*/) { return 0; };
  virtual G4VParticleChange *AtRestDoIt(const G4Track & /*track*/, const G4Step & /*stepData*/) { return 0; };
};

class G4UIcommand;
class G4UIdirectory;

/////////////////////
// Class Definition
/////////////////////

/*
  GLG4Scint is an extremely modified version of the G4Scintillation
  process, so much so that it's not even a G4Process anymore!
  Features include arbitrary scintillation light time profile and
  spectra, Birks' law, particle-dependent specification of all
  parameters, and reemission of optical photons killed by other processes.

    - Has a GenericPostPostStepDoIt() function (note two "Post"s)
      instead of a PostStepDoIt() function.  GenericPostPostStepDoIt()
      should be called by user in UserSteppingAction.  This guarantees
      that GLG4Scint will absolutely be the last process considered, and
      will definitely see the energy loss by charged particles accurately.

    - Modified to allow specification of absolute yield spectra,
      resolution scale, Birk's-law coefficient, and digitized waveform,
      customized for medium and (optionally) particle type.

    - No longer calls G4MaterialPropertiesTable::GetProperty() in
      [Post]PostStepDoit() -- all needed data can be found quickly in
      the internal physics table.

    - Uses poisson random distribution for number of photons if
      mean number of photons <= 12.

    - The total scintillation yield is now found implicitly from
      the integral of the scintillation spectrum, which must now be
      in units of photons per photon energy.

    - The above feature has been modified by Dario Motta: a scintillation yield
      CAN be defined and -if found- used instead of the implicit integral of the
      scintillation spectrum. This allows having scintillators with the same
      spectrum, but different light yields.

    - The materials property tables used are
        SCINTILLATION  ==  scintillation spectrum
        SCINTWAVEFORM  ==  scintillation waveform or time constant
        SCINTMOD       ==  resolution scale, Birk's constant, reference dE/dx

    - SCINTILLATION is required in each scintillating medium.
      (Okay to omit if you don't want the medium to scintillate.)

    - If SCINTWAVEFORM is missing, uses exponential waveform with default
      ScintillationTime.  If SCINTWAVEFORM contains negative "Momentum"'s
      then each "Momentum" is the decay time and its corresponding value
      is the relative strength of that exponential decay.
      Otherwise, the "PhotonEnergy" of each element is a time, and the
      Value of each element is the relative strength.

    - Default values of resolution scale (=1.0), Birk's constant (=0.0)
      and reference dE/dx (=0.0) are used if all or part of SCINTMOD is
      is missing.  SCINTMOD "PhotonEnergy" values should be set to the
      index number (0.0, 1.0, 2.0, with no units).

    - Birk's law (see 1998 Particle Data Booklet eq. 25.1) is implemented
      as
   yield(dE/dx) = yield_ref * dE/dx * (1 + kb*(dE/dx)_ref) / (1 + kb*(dE/dx)).
      I.e., the scintillation spectrum given in SCINTILLATION is
      measured for particles with dE/dx = (dE/dx)_ref.  The usual
      formula is recovered if (dE/dx)_ref = 0.0 (the default).
      This is useful if you have an empirically-measured spectrum for
      some densely-ionizing particle (like an alpha).

    - The constructor now accepts an additional string argument, tablename,
      which allows selection of alternate property tables.  E.g,
      tablename = "neutron" might be used to allow specification of a
      different waveform for scintillation due to neutron energy deposition.
      The code then searches for tables with names of the form
         "SCINTILLATIONneutron"
      If it finds such a table, that table is used in preference to
      the default (un-suffixed) table when stepping particles of that type.

    - The process generates at most maxTracksPerStep secondaries per step.
      If more "real" photons are needed, it increases the weight of the
      tracked opticalphotons.  Opticalphotons are thus macro-particles in
      the high-scintillation case.  The code preserves an integer number
      of real photons per macro-particle.
*/

class GLG4Scint : public G4UImessenger  // not creating a separate class is my laziness -GHS.
{
 private:
  //////////////
  // Operators
  //////////////

  // GLG4Scint& operator=(const GLG4Scint &right);

 public:
  ///////////////
  // Nested class
  ////////////////

  class MyPhysicsTable {
   public:
    G4String *fName;
    class Entry {
     public:
      G4PhysicsOrderedFreeVector *fSpectrumIntegral;
      G4PhysicsOrderedFreeVector *fReemissionIntegral;
      G4PhysicsOrderedFreeVector *fTimeIntegral;
      G4PhysicsOrderedFreeVector *fReemissionTimeIntegral;
      std::vector<G4PhysicsOrderedFreeVector *> fReemissionTimeVector;
      std::vector<G4PhysicsOrderedFreeVector *> fReemissionSpectrumVector;
      int fOwnSpectrumIntegral, fOwnTimeIntegral, fOwnReemissionTimeIntegral, fOwnReemissionTimeVector;
      G4double fResolutionScale;
      G4double fBirksConstant;
      G4double fRefdEdx;
      G4double fLightYield;
      G4MaterialPropertyVector *fQuenchingArray;

      Entry();
      ~Entry();

      void Build(const G4String &name, const G4String &matName, int material_index,
                 G4MaterialPropertiesTable *matprops);
    };

   private:
    static MyPhysicsTable *fHead;
    MyPhysicsTable *fNext;
    G4int fUsedByCount;

    Entry *fData;
    G4int fLength;

    MyPhysicsTable();
    ~MyPhysicsTable();

    void Build(const G4String &newname);

    friend class GLG4Scint;

   public:
    static MyPhysicsTable *FindOrBuild(const G4String &name);
    static const MyPhysicsTable *GetDefault(void) { return fHead; }
    void IncUsedBy(void) { ++fUsedByCount; }
    void DecUsedBy(void) {
      if (--fUsedByCount <= 0) delete this;
    }
    const Entry *GetEntry(int i) const { return fData + i; }
    void Dump(void) const;
  };

  ////////////////////////////////
  // Constructors and Destructor
  ////////////////////////////////

  GLG4Scint(const G4String &tableName = "", G4double lowerMassLimit = 0.0);

  // GLG4Scint(const GLG4Scint &right);

  ~GLG4Scint();

  ////////////
  // Methods
  ////////////

  G4VParticleChange *PostPostStepDoIt(const G4Track &aTrack, const G4Step &aStep);

  G4double GetLowerMassLimit(void) const;

  void DumpInfo() const;

  MyPhysicsTable *GetMyPhysicsTable(void) const;

  G4int GetVerboseLevel(void) const;
  void SetVerboseLevel(int level);

  // following two methods are for G4UImessenger
  void SetNewValue(G4UIcommand *command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand *command);

  ////////////////
  // static methods
  ////////////////

  static G4VParticleChange *GenericPostPostStepDoIt(const G4Step *pStep);

  // following are for energy deposition diagnosis
  static unsigned int fsScintillatedCount;
  static unsigned int fsReemittedCount;
  static void ResetPhotonCount(unsigned int sCt = 0, unsigned int rCt = 0) {
    fsScintillatedCount = sCt;
    fsReemittedCount = rCt;
  }
  static unsigned int GetScintillatedCount() { return fsScintillatedCount; }
  static unsigned int GetReemittedCount() { return fsReemittedCount; }

  static void GetTotEdepAll(G4double &totEdep, G4double &totEdepQuenched, G4double &totEdepTime,
                            G4ThreeVector &scintCentroidSum) {
    totEdep = fTotEdep;
    totEdepQuenched = fTotEdepQuenched;
    totEdepTime = fTotEdepTime;
    scintCentroidSum = fScintCentroidSum;
  }

  static void SetTotEdep(G4double &totEdep, G4double &totEdepQuenched, G4double &totEdepTime,
                         G4ThreeVector &scintCentroidSum) {
    fTotEdep = totEdep;
    fTotEdepQuenched = totEdepQuenched;
    fTotEdepTime = totEdepTime;
    fScintCentroidSum = scintCentroidSum;
  }

  static void ResetTotEdep() {
    fTotEdep = fTotEdepQuenched = fTotEdepTime = 0.0;
    fScintCentroidSum *= 0.0;
  }
  static G4double GetTotEdep() { return fTotEdep; }
  static G4double GetTotEdepQuenched() { return fTotEdepQuenched; }
  static G4double GetTotEdepTime() { return fTotEdepTime; }
  static G4bool GetDoScintillation() { return fDoScintillation; }
  static G4ThreeVector GetScintCentroidSum() { return fScintCentroidSum * (1.0 / fTotEdepQuenched); }

  ///////////////////////
  // Class Data Members
  ///////////////////////

 protected:
  int fVerboseLevel;

  // Below is the pointer to the physics table which this instance
  // of GLG4Scint will use.  You may create a separate instance
  // of GLG4Scint for each particle, if you like.
  MyPhysicsTable *fMyPhysicsTable;

  // below is the lower mass limit for this instance of GLG4Scint
  G4double fLowerMassLimit;

  // return value of PostPostStepDoIt
  G4ParticleChange fAParticleChange;

  ////////////////
  // static variables
  ////////////////

  // vector of all existing GLG4Scint objects.
  // They register themselves when created,
  // remove themselves when deleted.
  // Used by GenericPostPostStepDoIt
  static std::vector<GLG4Scint *> fMasterVectorOfGLG4Scint;

  // top level of scintillation command
  static G4UIdirectory *fGLG4ScintDir;

  // universal maximum number of secondary tracks per step for GLG4Scint
  static G4int fMaxTracksPerStep;

  // universal mean number of true photons per secondary track in GLG4Scint
  static G4double fMeanPhotonsPerSecondary;

  // universal on/off flag
  static G4bool fDoScintillation;

  // on/off flag for absorbed opticalphoton reemission
  static G4bool fDoReemission;

  QuenchingCalculator *fQuenching;

  // total real energy deposited and total quenched energy deposited
  static G4double fTotEdep;
  static G4double fTotEdepQuenched;
  static G4double fTotEdepTime;
  static G4ThreeVector fScintCentroidSum;

  // Bogus processes used to tag photons created in GLG4Scint
  static GLG4DummyProcess fScintProcess;
  static GLG4DummyProcess fReemissionProcess;
  static std::list<GLG4DummyProcess *> fReemissionProcessVector;

  // Quenching Factor
  static G4double fQuenchingFactor;
  // user-given (constant) quenching factor flag
  static G4bool fUserQF;
  static G4String fPrimaryName;
  // primary particle energy
  static G4double fPrimaryEnergy;

 public:
  // methods to access the Quenching Factor
  static G4double GetQuenchingFactor() { return fQuenchingFactor; }
  static void SetQuenchingFactor(G4double qf);
  // methods for getting/setting info for the QF calculation
  static G4double GetPrimaryEnergy() { return fPrimaryEnergy; }
  static void SetPrimaryEnergy(G4double pe) { fPrimaryEnergy = pe; }
  static G4String GetPrimaryName() { return fPrimaryName; }
  static void SetPrimaryName(G4String pn) { fPrimaryName = pn; }
};

////////////////////
// Inline methods
////////////////////

inline GLG4Scint::MyPhysicsTable *GLG4Scint::GetMyPhysicsTable() const { return fMyPhysicsTable; }

inline void GLG4Scint::DumpInfo() const {
  if (fMyPhysicsTable) {
    RAT::info << "GLG4Scint[" << *(fMyPhysicsTable->fName) << "] {" << newline
              << "  fLowerMassLimit=" << fLowerMassLimit << newline;
    if (fVerboseLevel >= 2) fMyPhysicsTable->Dump();
    RAT::info << "}" << newline;
  }
}

inline G4double GLG4Scint::GetLowerMassLimit(void) const { return fLowerMassLimit; }

inline void GLG4Scint::SetVerboseLevel(int level) { fVerboseLevel = level; }
inline G4int GLG4Scint::GetVerboseLevel(void) const { return fVerboseLevel; }

#endif /* GLG4Scint_h */
