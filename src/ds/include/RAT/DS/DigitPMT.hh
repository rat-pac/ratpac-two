/**
 * @class PMT
 * Data Structure: Output of analysis of digited PMT waveform
 *
 * This represents information about the digitzed PMT waveform in a detector event.
 */

#ifndef __RAT_DS_DigitPMT__
#define __RAT_DS_DigitPMT__

#include <Rtypes.h>
#include <TObject.h>

namespace RAT {
namespace DS {

/** Processed waveform information **/
class DigitPMT : public TObject {
 public:
  DigitPMT() : TObject() {}
  virtual ~DigitPMT() {}

  /** ID number of PMT */
  virtual void SetID(Int_t _id) { this->id = _id; }
  virtual Int_t GetID() { return id; }

  /** Threshold crossing time in ns */
  virtual void SetDigitizedTime(Double_t _dTime) { this->dTime = _dTime; }
  virtual Double_t GetDigitizedTime() { return dTime; }

  /** Integrated charge around the peak [pC] */
  virtual void SetDigitizedCharge(Double_t _dCharge) { this->dCharge = _dCharge; }
  virtual Double_t GetDigitizedCharge() { return dCharge; }

  /** Integrated total charge [pC] */
  virtual void SetDigitizedTotalCharge(Double_t _dTCharge) { this->dTCharge = _dTCharge; }
  virtual Double_t GetDigitizedTotalCharge() { return dTCharge; }

  /** Get the inter-sample timing interpolation */
  virtual void SetInterpolatedTime(Double_t _iTime) { this->iTime = _iTime; }
  virtual Double_t GetInterpolatedTime() { return iTime; }

  /** Get the sample associated with the crossing time */
  virtual void SetSampleTime(Int_t _sTime) { this->sTime = _sTime; }
  virtual Int_t GetSampleTime() { return sTime; }

  /** Total number of threshold crossings */
  virtual void SetNCrossings(Int_t _nCrossings) { this->nCrossings = _nCrossings; }
  virtual Int_t GetNCrossings() { return nCrossings; }

  /** Total time that the voltage over threshold */
  virtual void SetTimeOverThreshold(Double_t _timeOverThresh) { this->timeOverThresh = _timeOverThresh; }
  virtual Double_t GetTimeOverThreshold() { return timeOverThresh; }

  /** Sum of the voltage over threshold */
  virtual void SetVoltageOverThreshold(Double_t _voltageOverThresh) { this->voltageOverThresh = _voltageOverThresh; }
  virtual Double_t GetVoltageOverThreshold() { return voltageOverThresh; }

  /** Baseline in a pre-trigger window */
  virtual void SetPedestal(Double_t _pedestal) { this->pedestal = _pedestal; }
  virtual Double_t GetPedestal() { return pedestal; }

  /** Peak voltage */
  virtual void SetPeakVoltage(Double_t _peak) { this->peak = _peak; }
  virtual Double_t GetPeakVoltage() { return peak; }

  ClassDef(DigitPMT, 1);

 protected:
  Int_t id;
  Double_t dTime;
  Double_t dCharge;
  Double_t dTCharge;
  Double_t iTime;
  Int_t sTime;
  Int_t nCrossings;
  Double_t timeOverThresh;
  Double_t voltageOverThresh;
  Double_t pedestal;
  Double_t peak;
};

}  // namespace DS
}  // namespace RAT

#endif
