#ifndef __RAT_SplitEVDAQProc__
#define __RAT_SplitEVDAQProc__

#include <RAT/Processor.hh>
#include <RAT/DB.hh>

namespace RAT {


class SplitEVDAQProc : public Processor {
public:
  SplitEVDAQProc();
  virtual ~SplitEVDAQProc() { };
  virtual Processor::Result DSEvent(DS::Root *ds);
  void SetD(std::string param, double value);
  void SetI(std::string param, int value);

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
  double fDiscriminator;
  int fPmtType;
  int fTriggerOnNoise;
  std::vector<double> fSPECharge;
  DBLinkPtr ldaq;
};


} // namespace RAT

#endif
