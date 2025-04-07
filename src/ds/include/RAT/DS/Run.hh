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

#include <RAT/DS/ChannelStatus.hh>
#include <RAT/DS/NestedTubeInfo.hh>
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

  /** Nested tube information */
  virtual NestedTubeInfo *GetNestedTubeInfo() {
    if (nestedtubeinfo.empty()) {
      nestedtubeinfo.resize(1);
    }
    return &nestedtubeinfo[0];
  }
  virtual void SetNestedTubeInfo(const NestedTubeInfo *_nestedtubeinfo) {
    if (nestedtubeinfo.empty()) {
      nestedtubeinfo.resize(1);
    }
    nestedtubeinfo[0] = *_nestedtubeinfo;
  }
  virtual bool ExistNestedTubeInfo() { return !nestedtubeinfo.empty(); }
  virtual void PruneNestedTubeInfo() { nestedtubeinfo.resize(0); }

  /** Channel status */
  virtual ChannelStatus const *GetChannelStatus() const { return &ch_status; }
  virtual void SetChannelStatus(const ChannelStatus &_ch_status) { ch_status = _ch_status; }

  ClassDef(Run, 3);

 protected:
  Int_t id;
  ULong64_t type;
  TTimeStamp startTime;
  std::vector<NestedTubeInfo> nestedtubeinfo;
  std::vector<PMTInfo> pmtinfo;  // ah.. why is this a vector?
  ChannelStatus ch_status;
};

}  // namespace DS
}  // namespace RAT

#endif
