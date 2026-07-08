/** @class TrackSimplifyProc
 *  Simplifies MC particle tracks before output.
 *
 *  @author Scott DeGraw (scott.degraw@physics.ox.ac.uk)
 *
 *  Applies arc-length/time simplification (see TrackSimplifier.hh) to
 *  every track in the event, merging consecutive steps into buckets
 *  covering at least the configured path length or elapsed time. This is
 *  a pure I/O-side transform -- it runs after the simulation has produced
 *  the full-detail tracks, and only reduces how much of that detail gets
 *  written out.
 *
 *  Place this processor just before your output processor(s), e.g.
 *  @verbatim
 *    /rat/proc tracksimplify
 *    /rat/procset min_length 1.0
 *    /rat/procset min_time 0.05
 *    /rat/proc outntuple
 *  @endverbatim
 */

#ifndef __RAT_TrackSimplifyProc__
#define __RAT_TrackSimplifyProc__

#include <RAT/Processor.hh>

namespace RAT {

class TrackSimplifyProc : public Processor {
 public:
  TrackSimplifyProc();
  virtual ~TrackSimplifyProc() {}

  virtual Processor::Result DSEvent(DS::Root *ds);

  /** Set min_length (mm) or min_time (ns), accumulated per merged step. Values <= 0 disable that term. */
  virtual void SetD(std::string param, double value);

 protected:
  double fMinLength;
  double fMinTime;
};

}  // namespace RAT

#endif
