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

  virtual void SetPulseMean(double _fPulseMean) { fPulseMean = _fPulseMean; };
  virtual void SetPulseWidth(double _fPulseWidth) { fPulseWidth = _fPulseWidth; };

  virtual void SetPulseTimes(std::vector<double> _fPulseTimes) { fPulseTimes = _fPulseTimes; };
  virtual void SetPulseValues(std::vector<double> _fPulseValues) { fPulseValues = _fPulseValues; };

  virtual double GetPulseHeight(double time);
  virtual double GetPulseStartTime() { return fStartTime; };

 private:
  std::string fPulseType;
  std::string fPulseShape;

  double fPulseCharge;
  double fStartTime;
  double fPulseTimeOffset;
  double fPulseOffset;
  double fPulseMin;

  int fPulsePolaritySign;

  double fPulseMean;
  double fPulseWidth;

  std::vector<double> fPulseTimes;
  std::vector<double> fPulseValues;
};

}  // namespace RAT

#endif
