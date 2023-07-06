/**
 * @class LAPPD
 * Data Structure: LAPPD in triggered event
 *
 * This represents a LAPPD in a detector event.
 */

#ifndef __RAT_DS_LAPPD__
#define __RAT_DS_LAPPD__

#include <Rtypes.h>

#include <RAT/DS/LAPPDHit.hh>

namespace RAT {
namespace DS {

class LAPPD : public TObject {
 public:
  LAPPD() : TObject() {}
  virtual ~LAPPD() {}

  /** ID number of LAPPD */
  virtual void SetID(Int_t _id) { this->id = _id; }
  virtual Int_t GetID() { return id; }

  /** Total charge in waveform (pC) */
  virtual void SetTotalCharge(Double_t _charge) { this->totalcharge = _charge; }
  virtual Double_t GetTotalCharge() { return totalcharge; }

  /** Hit time in ns */
  virtual void SetTotalTime(Double_t _time) { this->totaltime = _time; }
  virtual Double_t GetTotalTime() { return totaltime; }

  /** List of hits in this LAPPD. */
  LAPPDHit *GetHit(Int_t i) { return &hits[i]; }
  Int_t GetNHits() const { return hits.size(); }
  LAPPDHit *AddNewHit() {
    hits.resize(hits.size() + 1);
    return &hits.back();
  }
  void PruneHits() { hits.resize(0); }

  ClassDef(LAPPD, 2);

 protected:
  Int_t id;
  std::vector<LAPPDHit> hits;
  Double_t totalcharge;
  Double_t totaltime;
};

}  // namespace DS
}  // namespace RAT

#endif
