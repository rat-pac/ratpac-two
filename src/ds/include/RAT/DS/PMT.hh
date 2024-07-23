/**
 * @class PMT
 * Data Structure: PMT in triggered event
 *
 * This represents a PMT in a detector event.
 */

#ifndef __RAT_DS_PMT__
#define __RAT_DS_PMT__

#include <Rtypes.h>

namespace RAT {
namespace DS {

class PMT : public TObject {
 public:
  PMT() : TObject() {}
  virtual ~PMT() {}

  /** ID number of PMT */
  virtual void SetID(Int_t _id) { this->id = _id; }
  virtual Int_t GetID() { return id; }

  /** Channel number of PMT */
  virtual void SetChannel(Int_t _ch) { this->ch = _ch; }
  virtual Int_t GetChannel() { return ch; }

  /** Total charge in waveform (pC) */
  virtual void SetCharge(Double_t _charge) { this->charge = _charge; }
  virtual Double_t GetCharge() { return charge; }

  /** Hit time in ns */
  virtual void SetTime(Double_t _time) { this->time = _time; }
  virtual Double_t GetTime() { return time; }

  /** Processed waveform time in ns */
  virtual void SetDigitizedTime(Double_t _dTime) { this->dTime = _dTime; }
  virtual Double_t GetDigitizedTime() { return dTime; }

  /** Processed waveform charge in pC */
  virtual void SetDigitizedCharge(Double_t _dCharge) { this->dCharge = _dCharge; }
  virtual Double_t GetDigitizedCharge() { return dCharge; }

  /** Processed waveform total charge in pC */
  virtual void SetDigitizedTotalCharge(Double_t _dTCharge) { this->dTCharge = _dTCharge; }
  virtual Double_t GetDigitizedTotalCharge() { return dTCharge; }

  /** Processed waveform timing in ns */
  virtual void SetInterpolatedTime(Double_t _iTime) { this->iTime = _iTime; }
  virtual Double_t GetInterpolatedTime() { return iTime; }

  virtual void SetSampleTime(Int_t _sTime) { this->sTime = _sTime; }
  virtual Int_t GetSampleTime() { return sTime; }

  virtual void SetNCrossings(Int_t _nCrossings) { this->nCrossings = _nCrossings; }
  virtual Int_t GetNCrossings() { return nCrossings; }

  virtual void SetTimeOverThreshold(Double_t _timeOverThresh) { this->timeOverThresh = _timeOverThresh; }
  virtual Double_t GetTimeOverThreshold() { return timeOverThresh; }

  virtual void SetPedestal(Double_t _pedestal) { this->pedestal = _pedestal; }
  virtual Double_t GetPedestal() { return pedestal; }

  virtual void SetPeakVoltage(Double_t _peak) { this->peak = _peak; }
  virtual Double_t GetPeakVoltage() { return peak; }

  ClassDef(PMT, 5);

 protected:
  Int_t id;
  Int_t ch;
  Double_t charge;
  Double_t time;
  Double_t dTime;
  Double_t dCharge;
  Double_t dTCharge;
  Double_t iTime;
  Int_t sTime;
  Int_t nCrossings;
  Double_t timeOverThresh;
  Double_t pedestal;
  Double_t peak;
};

}  // namespace DS
}  // namespace RAT

#endif
