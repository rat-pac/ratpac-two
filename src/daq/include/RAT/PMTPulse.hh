#ifndef __RAT_PMTPulse__
#define __RAT_PMTPulse__

#include <vector>

namespace RAT {

class PMTPulse {
 public:
  PMTPulse(){};
  virtual ~PMTPulse(){};

  virtual void SetPulseCharge(double _fPulseCharge) { fPulseCharge = _fPulseCharge; };
  virtual void SetPulseStartTime(double _fStartTime) { fStartTime = _fStartTime; };
  virtual void SetPulseMean(double _fPulseMean) { fPulseMean = _fPulseMean; };
  virtual void SetPulseWidth(double _fPulseWidth) { fPulseWidth = _fPulseWidth; };
  virtual void SetPulseOffset(double _fPulseOffset) { fPulseOffset = _fPulseOffset; };
  virtual void SetPulseTimeOffset(double _fPulseTimeOffset) { fPulseTimeOffset = _fPulseTimeOffset; };
  virtual void SetPulseMin(double _fPulseMin) { fPulseMin = _fPulseMin; };

  virtual double GetPulseHeight(double time);
  virtual double GetPulseStartTime() { return fStartTime; };

 private:
  double fPulseCharge;
  double fStartTime;
  double fPulseMean;
  double fPulseWidth;
  double fPulseTimeOffset;
  double fPulseOffset;
  double fPulseMin;
};

}  // namespace RAT

#endif
