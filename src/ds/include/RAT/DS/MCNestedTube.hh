/**
 * @class DS::MCNestedTube
 *  Data Structure: Hit NestedTube in Monte Carlo
 *
 *  @author Wilf Shorrock <w.shorrock@sussex.ac.uk>
 *
 *  This class represents a NestedTube in which at least one photon was captured
 */

#ifndef __RAT_DS_MCNestedTube__
#define __RAT_DS_MCNestedTube__

#include <RAT/DS/MCNestedTubeHit.hh>
#include <RAT/Log.hh>
#include <vector>

namespace RAT {
namespace DS {

class MCNestedTube : public TObject {
 public:
  MCNestedTube() : TObject() {}
  virtual ~MCNestedTube() {}

  /** ID number */
  virtual Int_t GetID() const { return id; };
  virtual void SetID(Int_t _id) { id = _id; };

  /** List of photons captured in this NestedTube. */
  MCNestedTubeHit *GetMCNestedTubeHit(Int_t i) { return &photon[i]; }
  Int_t GetMCNestedTubeHitCount() const { return photon.size(); }
  MCNestedTubeHit *AddNewMCNestedTubeHit() {
    photon.resize(photon.size() + 1);
    return &photon.back();
  }
  void RemoveMCNestedTubeHit(Int_t i) { photon.erase(photon.begin() + i); }
  void PruneMCNestedTubeHit() { photon.resize(0); }
  void SortMCNestedTubeHits() { std::sort(photon.begin(), photon.end()); }

  ClassDef(MCNestedTube, 3);

 protected:
  Int_t id;
  Int_t type;
  std::vector<MCNestedTubeHit> photon;
};

}  // namespace DS
}  // namespace RAT

#endif
