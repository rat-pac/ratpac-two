#ifndef __RAT_TrackInfo__
#define __RAT_TrackInfo__

#include <G4Allocator.hh>
#include <G4VUserTrackInformation.hh>
#include <RAT/CentroidCalculator.hh>
#include <cmath>
#include <map>
#include <string>

namespace RAT {

class TrackInfo : public G4VUserTrackInformation {
 public:
  TrackInfo(){};
  virtual ~TrackInfo(){};

  inline void *operator new(size_t);
  inline void operator delete(void *);

  void SetCreatorProcess(std::string &creatorProcess) { fCreatorProcess = creatorProcess; };
  void SetCreatorProcess(const char *creatorProcess) { fCreatorProcess = creatorProcess; };
  std::string GetCreatorProcess() const { return fCreatorProcess; };

  // Ok, I'm tired of getter/setter C++ bondage crap.  Just expose the
  // interface already.

  // G4 does not guaranttee that preUserTrackingAction is only called once on each track. Track this status to prevent
  // double counting
  bool preUserTrackingActionDone = false;

  /** Centroid of steps, weighted by energy loss. */
  CentroidCalculator energyCentroid;
  /** Centroid of optical photon creation vertices. */
  CentroidCalculator opticalCentroid;

  /** Energy lost by this track, indexed by volume name */
  std::map<std::string, double> energyLoss;

  /** Quenched energy deposit for the most recent step that went
   *  through GLG4Scint::PostPostStepDoIt, paired with that step's number so a
   *  step which never reached that code can be told apart from a zero deposit.
   **/
  double lastQuenchedEdep = 0.0;
  int lastQuenchedStepNumber = -1;

  /** Step in the parent track at which this track was created */
  void SetCreatorStep(int _CreatorStep) { CreatorStep = _CreatorStep; };
  int GetCreatorStep() const { return CreatorStep; };

  /** Global time at which the parent particle excited the
   *  scintillator, before any scintillation emission delay
   *  is added to produce this track's own creation time.
   **/
  void SetExcitationTime(double _excitationTime) { fExcitationTime = _excitationTime; };
  double GetExcitationTime() const { return fExcitationTime; };

  virtual void Print() const {};

 protected:
  std::string fCreatorProcess;
  int CreatorStep;
  double fExcitationTime = std::nan("");
};

// GEANT4 uses a custom allocator on subclass, so we need to override it here.
extern G4Allocator<TrackInfo> aTrackInfoAllocator;

inline void *TrackInfo::operator new(size_t) {
  void *aTrackInfo;
  aTrackInfo = (void *)aTrackInfoAllocator.MallocSingle();
  return aTrackInfo;
}

inline void TrackInfo::operator delete(void *aTrackInfo) { aTrackInfoAllocator.FreeSingle((TrackInfo *)aTrackInfo); }

}  // namespace RAT

#endif
