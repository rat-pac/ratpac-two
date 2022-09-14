/**
 * @class DS::Run
 * Run-level data structure
 *
 * @author Stan Seibert <sseibert@lanl.gov>
 */

#ifndef __RAT_DS_Run__
#define __RAT_DS_Run__

#include <TObject.h>
#include <TTimeStamp.h>

#include <RAT/DS/PMTInfo.hh>
#include <vector>

namespace RAT {
namespace DS {

class Run : public TObject {
 public:
  Run() : TObject() {}
  virtual ~Run() {}

  /** Run number. */
  virtual Int_t GetID() const { return id; }
  virtual void SetID(Int_t _id) { id = _id; }

  /** Run type bits */
  virtual ULong64_t GetType() const { return type; }
  virtual void SetType(ULong64_t _type) { type = _type; }

  /** Run start time */
  virtual TTimeStamp GetStartTime() const { return startTime; }
  virtual void SetStartTime(const TTimeStamp &_startTime) { startTime = _startTime; }

  /** PMT information */
  virtual PMTInfo *GetPMTInfo() {
    if (pmtinfo.empty()) {
      pmtinfo.resize(1);
    }
    return &pmtinfo[0];
  }
  virtual void SetPMTInfo(const PMTInfo *_pmtinfo) {
    if (pmtinfo.empty()) {
      pmtinfo.resize(1);
    }
    pmtinfo[0] = *_pmtinfo;
  }
  virtual bool ExistPMTInfo() { return !pmtinfo.empty(); }
  virtual void PrunePMTInfo() { pmtinfo.resize(0); }

  ClassDef(Run, 2)

      protected : Int_t id;
  ULong64_t type;
  TTimeStamp startTime;
  std::vector<PMTInfo> pmtinfo;
};

}  // namespace DS
}  // namespace RAT

#endif
