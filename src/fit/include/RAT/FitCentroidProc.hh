#ifndef __RAT_FitCentroidProc__
#define __RAT_FitCentroidProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitCentroidProc : public Processor {
 public:
  FitCentroidProc() : Processor("fitcentroid"), inputHandler(){};
  virtual ~FitCentroidProc() {}

  virtual void SetS(std::string param, std::string value);

  virtual void SetI(std::string param, int value);

  virtual void SetD(std::string param, double value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  std::vector<int> fPMTtype;   // Types of PMT to use in reconstruction.  If empty, uses all PMT types.
  std::string fFitLabel = "";  // Label for the fit result.  User can specify.
  double fPower = 2.0;         // Exponent to raise charge to when averaging.
  double fRescale = 1.0;
  FitterInputHandler inputHandler;
};

}  // namespace RAT

#endif  // __RAT_FitCentroidProc__
