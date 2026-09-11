#include <RAT/Log.hh>
#include <RAT/RatRunManager.hh>

namespace RAT {

void RatRunManager::BeamOn(G4int n_event, const char *macroFile, G4int n_select) {
  if (fNumEventsOverride >= 0) {
    if (fBeamOnCount > 0) {
      Log::Die(
          "-N/--num-events was given but /run/beamOn was invoked more than once in this "
          "invocation; the override would be ambiguous.");
    }
    n_event = fNumEventsOverride;
    fBeamOnCount++;
  }
  G4RunManager::BeamOn(n_event, macroFile, n_select);
}

}  // namespace RAT
