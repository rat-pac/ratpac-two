#ifndef __RAT_DSReader___
#define __RAT_DSReader___

#include <TChain.h>
#include <TObject.h>
#include <TTree.h>

#include <RAT/DB.hh>
#include <RAT/DS/Root.hh>
#include <string>

namespace RAT {

// Convenience class for ROOT scripts
class DSReader : public TObject {
 public:
  DSReader(const char *filename);
  virtual ~DSReader();

  // Reconstruct the global RAT::DB from the snapshot embedded in this file by
  // the outroot processor (the "ratdb" object). This restores the exact set of
  // tables used to build the geometry during production -- no $RATSHARE or geo
  // file needed. It is called automatically by the constructor so the DB is
  // ready before event looping begins, but is exposed so it can be re-run.
  // Returns the singleton DB instance.
  RAT::DB *LoadDB();

  // Construct and close the Geant4 detector geometry from the DB loaded by
  // LoadDB(), mirroring `rat`'s /run/initialize. This wraps RAT's G4 machinery
  // (DetectorConstruction, RunManager, PhysicsList) so no Geant4 types leak
  // into ROOT. Call once, after construction, before using the geometry (e.g.
  // the light-path calculator). Requires the libRATPAC library to be loaded.
  void BuildGeometry();

  void Add(const char *filename);
  void SetBranchStatus(const char *bname, bool status = 1) { T.SetBranchStatus(bname, status); };

  TTree *GetT() { return &T; };
  TTree *GetRunT() { return &runT; };
  virtual DS::Root *GetDS() { return ds; };
  Long64_t GetTotal() { return total; };

  // Load event.  Returns ds (which will now point to specified event)
  virtual DS::Root *GetEvent(Long64_t num) {
    next = num + 1;
    T.GetEntry(num);
    return ds;
  };
  virtual DS::Root *NextEvent();

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
