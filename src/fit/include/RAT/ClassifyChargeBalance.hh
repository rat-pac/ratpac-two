#ifndef __RAT_ClassifyChargeBalance__
#define __RAT_ClassifyChargeBalance__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class ClassifyChargeBalance : public Processor {
 public:
  ClassifyChargeBalance() : Processor("classifychargebalance"), inputHandler(){};
  virtual ~ClassifyChargeBalance() {}

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  std::vector<std::string> fLabels = {"chargebalance"};
  FitterInputHandler inputHandler;
};

}  // namespace RAT
#endif
