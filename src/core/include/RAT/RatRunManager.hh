/**
 * @class RAT::RatRunManager
 *
 * @detail RAT's G4RunManager subclass, the hook point for overriding Geant4
 * run behavior. Currently it lets the -N/--num-events command-line option
 * override the event count passed to every /run/beamOn.
 */

#ifndef __RAT_RatRunManager__
#define __RAT_RatRunManager__
#include <G4RunManager.hh>

namespace RAT {

class RatRunManager : public G4RunManager {
 public:
  void SetNumEventsOverride(G4int n) { fNumEventsOverride = n; }
  void BeamOn(G4int n_event, const char *macroFile = nullptr, G4int n_select = -1) override;

 private:
  G4int fNumEventsOverride = -1;
  G4int fBeamOnCount = 0;
};

}  // namespace RAT

#endif  // __RAT_RatRunManager__
