#ifndef __RAT_PMTPulse__
#define __RAT_PMTPulse__

#include <vector>

namespace RAT {

class PMTPulse {
 public:
  PMTPulse(std::string pulseType, std::string pulseShape);
  virtual ~PMTPulse(){};

  virtual void SetPulseCharge(double _fPulseCharge) { fPulseCharge = _fPulseCharge; };
  virtual void SetPulseStartTime(double _fStartTime) { fStartTime = _fStartTime; };
  virtual void SetPulseOffset(double _fPulseOffset) { fPulseOffset = _fPulseOffset; };
  virtual void SetPulseTimeOffset(double _fPulseTimeOffset) { fPulseTimeOffset = _fPulseTimeOffset; };
  virtual void SetPulseMin(double _fPulseMin) { fPulseMin = _fPulseMin; };
  virtual void SetPulsePolarity(bool _fPulsePolarity) { fPulsePolaritySign = _fPulsePolarity ? -1 : 1; };

  virtual void SetLogNPulseMean(double _fLogNPulseMean) { fLogNPulseMean = _fLogNPulseMean; };
  virtual void SetLogNPulseWidth(double _fLogNPulseWidth) { fLogNPulseWidth = _fLogNPulseWidth; };

  virtual void SetGausPulseWidth(double _fGausPulseWidth) { fGausPulseWidth = _fGausPulseWidth; };

  virtual void SetPulseShapeTimes(std::vector<double> _fPulseTimes) { fPulseTimes = _fPulseTimes; };
  virtual void SetPulseShapeValues(std::vector<double> _fPulseValues) { fPulseValues = _fPulseValues; };

  virtual double GetDataDrivenPulseVal(double time);

  virtual double GetPulseHeight(double time);
  virtual double GetPulseStartTimeNoOffset() { return fStartTime; };
  virtual double GetPulseStartTimeWithOffset() { return fStartTime + fPulseTimeOffset; };

 private:
  std::string fPulseType;
  std::string fPulseShape;

  double fPulseCharge;
  double fStartTime;
  double fPulseTimeOffset;
  double fPulseOffset;
  double fPulseMin;

  int fPulsePolaritySign;

  double fLogNPulseMean;
  double fLogNPulseWidth;

  double fGausPulseWidth;

  std::vector<double> fPulseTimes;
  std::vector<double> fPulseValues;
};

}  // namespace RAT

#endif
