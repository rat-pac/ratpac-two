// RAT::PileupGen
// Generates events with multiple primary vertices from a single
// vertex/position generator, with independently configurable
// (fixed/uniform/poisson) distributions for the number of vertices
// per event and their times.

#ifndef __PileupGen_h__
#define __PileupGen_h__

#include <cstdint>
#include <globals.hh>
#include <memory>
#include <vector>

#include "RAT/GLG4Gen.hh"

class GLG4TimeGen;
class GLG4VertexGen;
class GLG4PosGen;

namespace RAT {

class PileupMessenger;

// Distribution used for both the number-of-vertices-per-event and the
// per-vertex timing parameters of PileupGen.
enum PileupDistType { kPileupFixed, kPileupUniform, kPileupPoisson };

class PileupDist {
 public:
  PileupDist() : type(kPileupFixed), fixedValue(1.0), uniformMin(0.0), uniformMax(1.0), poissonMean(1.0){};

  // Format: "fixed:value" | "uniform:min:max" | "poisson:mean"
  void SetState(G4String state);
  G4String GetState() const;

  // Sorted event times for each vertex
  std::vector<double> GenerateTimes(uint64_t n, double startTime) const;
  // Non-negative integer sample, used for vertex multiplicity.
  uint64_t SampleMultiplicity() const;

  // True unless the configuration is provably able to sample a
  // multiplicity below 1 (fixed value or uniform range starting below 1).
  // Poisson multiplicity is always safe: SampleMultiplicity() rejects zero
  // samples and draws from a zero-truncated Poisson distribution instead.
  bool MultiplicityConfigIsValid() const;

 protected:
  PileupDistType type;
  // These are used as a continuous value, in ns by
  // GenerateTimes(), and rounded to the nearest integer count by
  // SampleMultiplicity()/MultiplicityConfigIsValid() -- the same double
  // does double duty as either a duration or a count depending on which
  // method the caller invokes. Same reasoning as poissonMean below.
  double fixedValue;
  double uniformMin, uniformMax;
  // Mean of a Poisson distribution when sampled by SampleMultiplicity()
  // (a discrete vertex count), or of the exponential distribution each
  // vertex's time offset is independently drawn from when sampled by
  // GenerateTimes().
  double poissonMean;
};

// Generates events made up of multiple primary vertices, (pileup) drawn
// from a single vertex/position generator configuration.  The number of
// vertices per event and their times relative to the event start time
// are each independently configurable as fixed, uniform, or Poisson-mean
// distributions.
class PileupGen : public GLG4Gen {
 public:
  PileupGen();
  virtual ~PileupGen();

  virtual void GenerateEvent(G4Event *event);
  virtual void ResetTime(double offset = 0.0);
  virtual bool IsRepeatable() const { return true; };

  virtual void SetState(G4String state);
  virtual G4String GetState() const;

  virtual void SetTimeState(G4String state);
  virtual G4String GetTimeState() const;
  virtual void SetVertexState(G4String state);
  virtual G4String GetVertexState() const;
  virtual void SetPosState(G4String state);
  virtual G4String GetPosState() const;

  void SetMultiplicityState(G4String state);
  G4String GetMultiplicityState() const;
  void SetVertexTimingState(G4String state);
  G4String GetVertexTimingState() const;

 protected:
  G4String stateStr;
  GLG4TimeGen *timeGen;
  GLG4VertexGen *vertexGen;
  GLG4PosGen *posGen;

  PileupDist multiplicityDist;
  PileupDist vertexTimingDist;

  std::unique_ptr<PileupMessenger> messenger;
};

}  // namespace RAT

#endif
