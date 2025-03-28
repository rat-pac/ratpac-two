#ifndef __RAT_GeoFiberSensitiveDetector__
#define __RAT_GeoFiberSensitiveDetector__

#include <G4VSensitiveDetector.hh>
#include <RAT/GeoFiberSensitiveDetectorHit.hh>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace RAT {

class GeoFiberSensitiveDetector : public G4VSensitiveDetector {
 public:
  GeoFiberSensitiveDetector(G4String name);
  virtual ~GeoFiberSensitiveDetector();

  virtual void Initialize(G4HCofThisEvent *HCE);
  virtual G4bool ProcessHits(G4Step *aStep, G4TouchableHistory *ROhist);
  virtual void EndOfEvent(G4HCofThisEvent *HCE);

  // Data members which are publicly accessible and can be
  // written out to the RAT event tree

  std::vector<double> _hit_x;
  /** hit x-coordinate */
  std::vector<double> _hit_y;
  /** hit y-coordinate */
  std::vector<double> _hit_z;
  /** hit z-coordinate */
  std::vector<double> _hit_E;
  /** hit energy deposition */
  std::vector<double> _hit_time;
  /** global time of the hit */
  std::vector<int> _hit_uid;
  /** unique identifier code of the hit */
  std::vector<int> _hit_pdg;
  /** pdg of particle that left the hit */
  std::vector<std::string> _hit_volume;
  /** name of volume of hit */

 private:
  int fLastEventID;
  int fLastTrackID;

  GeoFiberSensitiveDetectorHitsCollection *_hitsCollection;
  G4int HCID;
  G4HCofThisEvent *_HCE;
};

}  // namespace RAT

#endif
