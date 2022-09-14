#ifndef __RAT_FitCentroidProc__
#define __RAT_FitCentroidProc__

#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitCentroidProc : public Processor {
 public:
  FitCentroidProc();
  virtual ~FitCentroidProc() {}

  /** param = "power", value = exponent to raise charge to when averaging
   *  default is 2.0 */
  virtual void SetD(std::string param, double value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  double fPower;
  double fRescale;
};

}  // namespace RAT

#endif  // __RAT_FitCentroidProc__
