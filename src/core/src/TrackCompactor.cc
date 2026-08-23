#include <RAT/TrackCompactor.hh>

namespace RAT {

TrackCompactor::TrackCompactor(DS::MCTrack *track, double minLength, double minTime)
    : fTrack(track), fMinLength(minLength), fMinTime(minTime), fBucketStartTime(0.0), fBucketOpen(false) {}

void TrackCompactor::AddInitialStep(const DS::MCTrackStep &step) {
  *fTrack->AddNewMCTrackStep() = step;
  fBucketStartTime = step.GetGlobalTime();
  fBucketOpen = false;
}

void TrackCompactor::AddStep(const DS::MCTrackStep &step, bool closeBucket) {
  if (!IsEnabled()) {
    *fTrack->AddNewMCTrackStep() = step;
    return;
  }

  DS::MCTrackStep *bucket;
  if (!fBucketOpen) {
    bucket = fTrack->AddNewMCTrackStep();
    *bucket = step;
    fBucketOpen = true;
  } else {
    bucket = fTrack->GetLastMCTrackStep();
    const double length = bucket->GetLength() + step.GetLength();
    const double depositedEnergy = bucket->GetDepositedEnergy() + step.GetDepositedEnergy();
    const double quenchedDepositedEnergy = bucket->GetQuenchedDepositedEnergy() + step.GetQuenchedDepositedEnergy();

    *bucket = step;
    bucket->SetLength(length);
    bucket->SetDepositedEnergy(depositedEnergy);
    bucket->SetQuenchedDepositedEnergy(quenchedDepositedEnergy);
  }

  const bool lengthExceeded = fMinLength > 0.0 && bucket->GetLength() >= fMinLength;
  const bool timeExceeded = fMinTime > 0.0 && bucket->GetGlobalTime() - fBucketStartTime >= fMinTime;
  if (lengthExceeded || timeExceeded || closeBucket) {
    fBucketStartTime = bucket->GetGlobalTime();
    fBucketOpen = false;
  }
}

bool TrackCompactor::IsEnabled() const { return fMinLength > 0.0 || fMinTime > 0.0; }

}  // namespace RAT
