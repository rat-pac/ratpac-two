#ifndef __RAT_TrueDAQProc__
#define __RAT_TrueDAQProc__

#include <RAT/DB.hh>
#include <RAT/Processor.hh>

namespace RAT {

class TrueDAQProc : public Processor {
 public:
  TrueDAQProc();
  virtual ~TrueDAQProc(){};
  virtual Processor::Result DSEvent(DS::Root *ds);
  void SetD(std::string param, double value);
  void SetI(std::string param, int value);

 protected:
  int fEventCounter;
  double fTriggerWindow;
  double fTriggerLockout;
  double fMaxHitTime;
  DBLinkPtr ldaq;
};

}  // namespace RAT

#endif
