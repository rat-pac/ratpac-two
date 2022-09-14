// fsutanto@umich.edu
// akindele1@llnl.gov
// March 9, 2020

// To use this generator, the command is:
// /generator/add n ISOTOPE:POSITION[:TIME]
// For example:
// /generator/add n 17:fill

#ifndef __RAT_NGen__
#define __RAT_NGen__

#include <RAT/GLG4Gen.hh>
#include <Randomize.hh>
#include <globals.hh>

class G4Event;
class G4ParticleDefinition;
class GLG4TimeGen;
class GLG4PosGen;

namespace RAT {

class NGen : public GLG4Gen {
 public:
  NGen();
  virtual ~NGen();
  virtual void GenerateEvent(G4Event *event);
  virtual void ResetTime(double offset = 0.0);
  virtual bool IsRepeatable() const { return true; };

  virtual void SetState(G4String state);
  virtual G4String GetState() const;

  virtual void SetTimeState(G4String state);
  virtual G4String GetTimeState() const;
  virtual void SetPosState(G4String state);
  virtual G4String GetPosState() const;

 protected:
  // Generator initialization, specified by the user.
  G4String stateStr;

  // isotope.
  int isotope;

  // The time and position generators specified by the user.
  GLG4TimeGen *timeGen;
  GLG4PosGen *posGen;

  // The N17Source event model only generate neutrons and photons.
  G4ParticleDefinition *neutron;
  G4ParticleDefinition *electron;

 private:
  G4RandGeneral *spectrumSampler;
  virtual void SetUpBetaSpectrumSampler(G4double &e0);
  virtual G4double FermiFunction(G4double &W);
  virtual G4double ModSquared(G4double &re, G4double &im);

  G4int Z;
  G4int A;
  G4double alphaZ;
  G4double Rnuc;
  G4double V0;
  G4double gamma0;
  G4double sumBr;
  G4double pdfNow;
};

}  // namespace RAT

#endif  // RAT_NGen_h
