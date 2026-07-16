#ifndef __RAT_DSReader___
#define __RAT_DSReader___

#include <TChain.h>
#include <TObject.h>
#include <TTree.h>

#include <RAT/DB.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <string>

namespace RAT {

// Convenience class for ROOT scripts
class DSReader : public TObject {
 public:
  DSReader(const std::string &filename);
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

  void Add(const std::string &filename);
  void SetBranchStatus(const std::string &bname, bool status = 1) { T.SetBranchStatus(bname.c_str(), status); };

  TTree *GetT() { return &T; };
  TTree *GetRunT() { return &runT; };
  virtual DS::Root *GetDS() { return fDS; };
  Long64_t GetEntryCount() { return fTotalEntries; };

  virtual bool HasNextEntry() { return next < fTotalEntries; };
  virtual size_t GetRunCount() { return fTotalRuns; };

  // FIXME: return types for all functions below should be const, but we don't
  // have enough downstream const accessors to make this work yet.

  // Load event.  Returns ds (which will now point to specified event)
  virtual DS::Root &GetEntry(Long64_t num) {
    next = num + 1;
    T.GetEntry(num);
    return *fDS;
  };

  virtual DS::Root &NextEntry();
  virtual DS::Run &GetRun() { return GetRunByRunID(fDS->GetRunID()); };
  virtual DS::Run &GetRunByRunID(int runID);
  virtual DS::Run &GetRunByIndex(size_t index);
  ClassDef(DSReader, 1);

 protected:
  TChain T;
  TChain runT;
  DS::Root *fDS;
  DS::Run *fRun;
  size_t next;
  size_t fTotalEntries;
  size_t fTotalRuns;
};

}  // namespace RAT

#endif
