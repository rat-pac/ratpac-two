#ifndef __RAT_FitMimirProc__
#define __RAT_FitMimirProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <mimir/FitStrategy.hh>

namespace RAT {

class FitMimirProc : public Processor {
 public:
  FitMimirProc();
  virtual ~FitMimirProc() {}
  void BeginOfRun(DS::Run *run) override;
  // virtual void SetI(std::string param, int value) override;
  // virtual void SetD(std::string param, double value) override;
  Processor::Result Event(DS::Root *ds, DS::EV *ev) override;
  void Configure(const std::string &index = "");

 protected:
  FitterInputHandler inputHandler;
  std::unique_ptr<Mimir::FitStrategy> strategy;
};

}  // namespace RAT

#endif
