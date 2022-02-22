#ifndef __RAT_SplitEVDAQProc__
#define __RAT_SplitEVDAQProc__

#include <RAT/Processor.hh>
#include <RAT/DB.hh>
#include <RAT/PMTWaveform.hh>
#include <RAT/DS/MCPMT.hh>
#include <RAT/Digitizer.hh>
#include <RAT/DS/Digit.hh>
#include <string>

namespace RAT {

class SplitEVDAQProc : public Processor {

  public:
    SplitEVDAQProc();
    virtual ~SplitEVDAQProc() { };
    virtual Processor::Result DSEvent(DS::Root *ds);
    void SetD(std::string param, double value);
    void SetI(std::string param, int value);

  protected:

    PMTWaveform GenerateWaveforms(DS::MCPMT* mcpmt);

    int fEventCounter;
    double fPulseWidth;
    double fTriggerThreshold;
    double fTriggerWindow;
    double fPmtLockout;
    double fTriggerLockout;
    double fTriggerResolution;
    double fLookback;
    double fMaxHitTime;
    int fPmtType;
    int fTriggerOnNoise;
    DBLinkPtr ldaq;

    Digitizer *fDigitizer;
    std::string fDigitizerType;
    bool fDigitize;

    DS::Digit digit;

    DBLinkPtr lpulse;
    double fPMTPulseMin;
    double fPMTPulseOffset;
    double fPMTPulseTimeOffset;
    double fPMTPulseWidth;
    double fPMTPulseMean;
  };

} // namespace RAT

#endif
