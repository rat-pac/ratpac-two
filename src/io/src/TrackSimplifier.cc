#include <RAT/TrackSimplifier.hh>
#include <cstddef>
#include <vector>

namespace RAT {

void SimplifyTrack(DS::MCTrack *track, double minLength, double minTime) {
  const size_t n = track->GetMCTrackStepCount();
  if (n <= 2 || (minLength <= 0.0 && minTime <= 0.0)) return;

  std::vector<DS::MCTrackStep> kept;

  double accLength = 0.0;
  double accEnergy = 0.0;
  double accQuenchedEnergy = 0.0;
  double bucketStartTime = track->GetMCTrackStep(0)->GetGlobalTime();

  for (size_t i = 0; i < n; ++i) {
    const DS::MCTrackStep &s = *track->GetMCTrackStep(i);

    accLength += s.GetLength();
    accEnergy += s.GetDepositedEnergy();
    accQuenchedEnergy += s.GetQuenchedDepositedEnergy();

    const bool lengthExceeded = (minLength > 0.0) && (accLength >= minLength);
    const bool timeExceeded = (minTime > 0.0) && (s.GetGlobalTime() - bucketStartTime >= minTime);
    const bool isLast = (i + 1 == n);

    if (i == 0 || lengthExceeded || timeExceeded || isLast) {
      kept.push_back(s);
      kept.back().SetLength(accLength);
      kept.back().SetDepositedEnergy(accEnergy);
      kept.back().SetQuenchedDepositedEnergy(accQuenchedEnergy);
      accLength = 0.0;
      accEnergy = 0.0;
      accQuenchedEnergy = 0.0;
      bucketStartTime = s.GetGlobalTime();
    }
  }

  track->PruneMCTrackStep();
  for (const DS::MCTrackStep &s : kept) *track->AddNewMCTrackStep() = s;
}

}  // namespace RAT
