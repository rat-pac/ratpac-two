#include <RAT/TrackSimplifier.hh>
#include <RAT/TrackSimplifyProc.hh>

namespace RAT {

TrackSimplifyProc::TrackSimplifyProc() : Processor("tracksimplify"), fMinLength(0.0), fMinTime(0.0) {}

void TrackSimplifyProc::SetD(std::string param, double value) {
  if (param == "min_length")
    fMinLength = value;
  else if (param == "min_time")
    fMinTime = value;
  else
    throw ParamUnknown(param);
}

Processor::Result TrackSimplifyProc::DSEvent(DS::Root *ds) {
  if (!ds->ExistMC()) return Processor::OK;

  DS::MC *mc = ds->GetMC();
  for (int i = 0; i < mc->GetMCTrackCount(); i++) {
    SimplifyTrack(mc->GetMCTrack(i), fMinLength, fMinTime);
  }

  return Processor::OK;
}

}  // namespace RAT
