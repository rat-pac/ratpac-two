#ifndef __RAT_FitTensorProc__
#define __RAT_FitTensorProc__

#if TENSORFLOW_Enabled
#include <cppflow/cppflow.h>

#include <RAT/DS/PMTInfo.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitTensorProc : public Processor {
 public:
  FitTensorProc();
  virtual ~FitTensorProc() {}

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);
  TVector3 PositionFit(DS::Root *ds, DS::EV *ev);
  TVector3 DirectionFit(DS::Root *ds, DS::EV *ev, TVector3 pos);

  cppflow::tensor CreateProjection(DS::EV *ev, DS::PMTInfo *pmtinfo, TVector3 coordinates);
  void BeginOfRun(DS::Run *run);

 protected:
  double fPower;
  double fRescale;
  cppflow::model *positionModel;
  cppflow::model *directionModel;
};

}  // namespace RAT

#endif
#endif  // __RAT_FitTensorProc__
