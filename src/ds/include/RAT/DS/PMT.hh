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

  /** Total charge in waveform (pC) */
  virtual void SetCharge(Float_t _charge) { this->charge = _charge; }
  virtual Float_t GetCharge() { return charge; }

  /** Hit time in ns */
  virtual void SetTime(Double_t _time) { this->time = _time; }
  virtual Double_t GetTime() { return time; }

  /** Processed waveform time in ns */
  virtual void SetDigitizedTime(Double_t _dTime) { this->dTime = _dTime; }
  virtual Double_t GetDigitizedTime() { return dTime; }

  /** Processed waveform time in ns */
  virtual void SetDigitizedCharge(Double_t _dCharge) { this->dCharge = _dCharge; }
  virtual Double_t GetDigitizedCharge() { return dCharge; }

  ClassDef(PMT, 2);

 protected:
  Int_t id;
  Float_t charge;
  Double_t time;
  Double_t dTime;
  Double_t dCharge;
};

}  // namespace DS
}  // namespace RAT

#endif
