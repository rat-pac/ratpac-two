/** @file TrackCompactor.hh
 *  Online arc-length/time based compaction of DS::MCTrack.
 */

#ifndef __RAT_TrackCompactor__
#define __RAT_TrackCompactor__

#include <RAT/DS/MCTrack.hh>

namespace RAT {

/**
 * Incrementally compact a track while its steps are being produced.
 *
 * The most recent output step is kept as an open bucket and updated in place
 * until the length or time threshold is reached. A caller can also force a
 * bucket to close, for example at a geometry boundary. The initial track step
 * is stored separately and is never merged with a transport step.
 */
class TrackCompactor {
 public:
  TrackCompactor(DS::MCTrack *track, double minLength, double minTime);

  /** Store the zero-length step marking the start of the track. */
  void AddInitialStep(const DS::MCTrackStep &step);

  /**
   * Add a transport step, merging it into the current bucket when possible.
   *
   * @param step         Step to add.
   * @param closeBucket  Close the bucket after this step regardless of its accumulated length or time.
   */
  void AddStep(const DS::MCTrackStep &step, bool closeBucket = false);

 private:
  bool IsEnabled() const;

  DS::MCTrack *fTrack;
  double fMinLength;
  double fMinTime;
  double fBucketStartTime;
  bool fBucketOpen;
};

}  // namespace RAT

#endif
