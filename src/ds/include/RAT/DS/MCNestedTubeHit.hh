/**
 * @class DS::MCNestedTubeHit
 * Data Structure: Photon captured in a nested tube (optical fiber).
 *
 * @author Wilf Shorrock <w.shorrock@sussex.ac.uk>
 *
 * This class represents a single photon that is captured within the core of a
 * nested tube object.
 */

#ifndef __RAT_DS_MCNestedTubeHit__
#define __RAT_DS_MCNestedTubeHit__

#include <TObject.h>
#include <TVector3.h>

namespace RAT {
namespace DS {

class MCNestedTubeHit : public TObject {
 public:
  MCNestedTubeHit() : TObject() {}
  virtual ~MCNestedTubeHit() {}

  /** Time of photon hit on nested tube relative to event start time (ns). */
  virtual Double_t GetHitTime() const { return hitTime; }
  virtual void SetHitTime(Double_t _hitTime) { hitTime = _hitTime; }

  /** Location of photon hit (mm). */
  virtual TVector3 GetPosition() const { return pos; }
  virtual void SetPosition(const TVector3 &_pos) { pos = _pos; }

  /** Hit ID of photon */
  virtual void SetHitID(Int_t _hitID) { hitID = _hitID; }
  virtual Int_t GetHitID() const { return hitID; }

  /** Operator overload **/
  bool operator<(const MCNestedTubeHit &mcp) const { return (hitTime < mcp.hitTime); }
  bool operator>(const MCNestedTubeHit &mcp) const { return (hitTime > mcp.hitTime); }

  ClassDef(MCNestedTubeHit, 4);

 protected:
  Double_t hitTime;
  TVector3 pos;

  Int_t hitID;
  Int_t fiberID;
};

}  // namespace DS
}  // namespace RAT

#endif
