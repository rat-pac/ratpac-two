#ifndef __RAT_SplitEVDAQProc__
#define __RAT_SplitEVDAQProc__

#include <RAT/DB.hh>
#include <RAT/Digitizer.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

class SplitEVDAQProc : public Processor {
 public:
  SplitEVDAQProc();
  virtual ~SplitEVDAQProc(){};
  virtual Processor::Result DSEvent(DS::Root *ds);
  void SetD(std::string param, double value);
  void SetI(std::string param, int value);

  void BeginOfRun(DS::Run *run);

 protected:
  int fEventCounter;
  double fPulseWidth;
  double fTriggerThreshold;
  double fTriggerWindow;
  double fPmtLockout;
  double fTriggerLockout;
  double fTriggerResolution;
  double fLookback;
  double fMaxHitTime;
  bool fDigitize;

  int fTriggerOnNoise;
  DBLinkPtr ldaq;

  Digitizer *fDigitizer;
  std::string fDigitizerType;
};

}  // namespace RAT

#endif
