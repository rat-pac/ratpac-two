#ifndef __RAT_ForcedTriggerProc__
#define __RAT_ForcedTriggerProc__

#include <RAT/DB.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

class ForcedTriggerProc : public Processor {
 public:
  ForcedTriggerProc();
  virtual ~ForcedTriggerProc(){};
  virtual Processor::Result DSEvent(DS::Root *ds);

  void BeginOfRun(DS::Run *run);

 protected:
  int fEventCounter;
  bool fDigitize;

  DBLinkPtr ldaq;

  Digitizer *fDigitizer;
  std::string fDigitizerType;
};

}  // namespace RAT

#endif
