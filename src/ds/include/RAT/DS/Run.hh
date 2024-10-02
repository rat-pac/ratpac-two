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
  virtual void SetStartTime(const TTimeStamp& _startTime) { startTime = _startTime; }

  /** PMT information */
  virtual PMTInfo*& GetPMTInfo() {
    /*if (pmtinfo.empty()) {
      std::cout << "Error: pmtinfo is empty" << std::endl;
      pmtinfo.resize(1);
      std::cout << "Resized pmtinfo to 1" << std::endl;
    }*/
    // std::cout << "Printing the address of pmtinfo " << &pmtinfo << std::endl;
    return pmtinfo;
  }
  virtual void SetPMTInfo(PMTInfo* _pmtinfo) {
    std::cout << "Setting PMTInfo " << std::endl;

    if (_pmtinfo == nullptr) {
      std::cout << "Error: _pmtinfo is a null pointer" << std::endl;
      return;
    }

    /*if (pmtinfo.empty()) {
        pmtinfo.resize(1);
      }*/
    std::cout << "Address of _pmtinfo: " << _pmtinfo << std::endl;
    std::cout << "Address of pmtinfo[0] before assignment: " << &pmtinfo[0] << std::endl;
    std::cout << " Address of pmtinfo vector: " << &pmtinfo << std::endl;
    std::cout << _pmtinfo;

    pmtinfo = _pmtinfo;
    // pmtinfo[0] = _pmtinfo;
    std::cout << "Assigned  pmtinfo[0] = *_pmtinfo " << std::endl;
  }
  virtual bool ExistPMTInfo() { return pmtinfo != nullptr; }  // pmtinfo ? true : false; } //!pmtinfo.empty(); }
  virtual void PrunePMTInfo() {}                              // pmtinfo.resize(0); }

  ClassDef(Run, 2);

 protected:
  Int_t id;
  ULong64_t type;
  TTimeStamp startTime;
  PMTInfo* pmtinfo;
  // std::vector<PMTInfo*> pmtinfo;
};

}  // namespace DS
}  // namespace RAT

#endif
