#ifndef __RAT_EventInfo__
#define __RAT_EventInfo__

#include <G4VUserEventInformation.hh>
#include <RAT/CentroidCalculator.hh>
#include <RAT/DS/Calib.hh>
#include <map>
#include <string>

namespace RAT {

// Holds extra event information inside the G4Event
class EventInfo : public G4VUserEventInformation {
 public:
  EventInfo() {
    fCalib = new DS::Calib();  // FIXME
    numScintPhoton = 0;
    numReemitPhoton = 0;
    numCerenkovPhoton = 0;
    extTriggerType = 0;
    extTriggerTime = 0.0;
  };

  virtual ~EventInfo() { delete fCalib; }

  virtual void Print() const {}

  void SetCalib(DS::Calib *calib) {
    delete fCalib;
    fCalib = calib;
  }
  const DS::Calib *GetCalib() const { return fCalib; }

  // Ok, I'm tired of getter/setter C++ bondage crap.  Just expose the
  // interface already.

  /** Centroid of steps, weighted by energy loss. */
  CentroidCalculator energyCentroid;

  /** Centroid of optical photon creation vertices. */
  CentroidCalculator opticalCentroid;

  /** Energy lost by this event, indexed by volume name */
  std::map<std::string, double> energyLoss;

  /** UTC Time of this Monte Carlo event */
  TTimeStamp utc;

  /** Count of scintillation photons in this event */
  int numScintPhoton;

  /** Count of reemitted photons in this event */
  int numReemitPhoton;

  /** Count the number of Cerenkov photon in this event */
  int numCerenkovPhoton;
  /** Type of external trigger associated with this event (0 for none) */
  int extTriggerType;

  /** Time of external trigger relative to start of event primaries */
  double extTriggerTime;

  bool StorePhotonIDs;

  bool StoreCapture158GdIDs;

  /** Map of photon track ID to original parent ID and creation step */
  std::map<int, std::vector<int>> PhotonIDParentStep;

  /** Map of neutron capture on 158Gd's secondaries track ID to
   original parent ID and creation step */
  std::map<int, std::vector<int>> Capture158GdIDParentStep;

  /**  Vector to store time/photon information */
  std::vector<G4double> timePhotonID;                   // mfb
  std::vector<std::vector<G4double>> timePhotonMatrix;  // mfb

 protected:
  DS::Calib *fCalib;
};

}  // namespace RAT

#endif
