// RAT::AmBeGen
// 10-Jan-2006 WGS

// Implements a GLG4Sim-style generator for the amBe reaction.  The class we use
// is AmBeSource, copied from the CfSource class by Vincent Fischer.

// To use this generator, the command is:

// /generator/add ambe POSITION[:TIME]

// (that is, the TIME is optional).  POSITION and TIME are the
// same as for the combo generator.  For example:

// /generator/add AmBe:fill

// Note that there is also no "defering" of any tracks of emitted
// particles into other events.  "TIME" refers to the t0 of the
// fission; the default is flat time distribution with a rate of 1 ns.

#ifndef __RAT_AmBeGen__
#define __RAT_AmBeGen__

#include <RAT/GLG4Gen.hh>
#include <globals.hh>

class G4Event;
class G4ParticleDefinition;
class GLG4TimeGen;
class GLG4PosGen;

namespace RAT {

class AmBeGen : public GLG4Gen {
 public:
  AmBeGen();
  virtual ~AmBeGen();
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

  // The time and position generators specified by the user.
  GLG4TimeGen *timeGen;
  GLG4PosGen *posGen;

  // The AmBeSource event model only generate neutrons and photons.
  G4ParticleDefinition *neutron;
  G4ParticleDefinition *gamma;
};

}  // namespace RAT

#endif  // RAT_AmBeGen_h
