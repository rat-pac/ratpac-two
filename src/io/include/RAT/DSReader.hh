#ifndef __RAT_DSReader___
#define __RAT_DSReader___

#include <TChain.h>
#include <TObject.h>
#include <TTree.h>

#include <RAT/DS/Root.hh>
#include <string>

namespace RAT {

// Convenience class for ROOT scripts
class DSReader : public TObject {
 public:
  DSReader(const char *filename);
  virtual ~DSReader();

  void Add(const char *filename);
  void SetBranchStatus(const char *bname, bool status = 1) { T.SetBranchStatus(bname, status); };

  TTree *GetT() { return &T; };
  TTree *GetRunT() { return &runT; };
  DS::Root *GetDS() { return ds; };
  Long64_t GetTotal() { return total; };

  // Load event.  Returns ds (which will now point to specified event)
  DS::Root *GetEvent(Long64_t num) {
    next = num + 1;
    T.GetEntry(num);
    return ds;
  };
  DS::Root *NextEvent();

  ClassDef(DSReader, 0);

 protected:
  TChain T;
  TChain runT;
  DS::Root *ds;
  Long64_t next;
  Long64_t total;
};

}  // namespace RAT

#endif
