// fsutanto@umich.edu
// Apr 15, 2018
// almost copy pasted from CfGen.hh

#ifndef __RAT_GdGen__
#define __RAT_GdGen__

#include <RAT/DB.hh>
#include <RAT/GLG4Gen.hh>
#include <globals.hh>

class G4Event;
class G4ParticleDefinition;
class GLG4TimeGen;
class GLG4PosGen;

namespace RAT {

class GdGen : public GLG4Gen {
 public:
  GdGen();
  virtual ~GdGen();

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

  // Gd158 isotope.
  int isotope;

  // The time and position generators specified by the user.
  GLG4TimeGen *timeGen;
  GLG4PosGen *posGen;

  // The Gd158Source event model only generate electrons and photons.
  G4ParticleDefinition *electron;
  G4ParticleDefinition *gamma;

  // Secondaries data from the database
  std::vector<int> thePar, theMul;
  std::vector<double> theErg, theCdf;
  int theMax;
};

}  // namespace RAT

#endif  // RAT_GdGen_h
