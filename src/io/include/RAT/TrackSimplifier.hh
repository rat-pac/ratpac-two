/** @file TrackSimplifier.hh
 *  Arc-length/time based simplification of DS::MCTrack.
 *
 *  Long tracks can carry thousands of steps that add little information
 *  beyond the overall shape of the trajectory. SimplifyTrack() thins a track
 *  down by merging consecutive steps into buckets bounded by a minimum path
 *  length and/or elapsed time.
 */

#ifndef __RAT_TrackSimplifier__
#define __RAT_TrackSimplifier__

#include <RAT/DS/MCTrack.hh>

namespace RAT {

/**
 * Simplify @p track in place by merging consecutive steps into buckets,
 * each covering at least @p minLength of path length or @p minTime of
 * elapsed time (whichever is reached first) before being emitted as a
 * single step.
 *
 * Steps within a bucket have their length and (quenched) deposited energy
 * summed into the single step emitted for that bucket, so track-level
 * totals measured by summing over steps are unaffected by simplification.
 * The emitted step's position and time are those of the last step in the
 * bucket. The first step of the track is always kept as-is, since it marks
 * the track's starting point and carries zero length/energy.
 *
 * Does nothing if the track has 2 or fewer steps, or if both @p minLength
 * and @p minTime are <= 0.
 *
 * @param track      Track to simplify, modified in place.
 * @param minLength  Path length (mm) accumulated per merged step. Values <= 0 disable this term.
 * @param minTime    Elapsed time (ns) accumulated per merged step. Values <= 0 disable this term.
 */
void SimplifyTrack(DS::MCTrack *track, double minLength, double minTime);

}  // namespace RAT

#endif
