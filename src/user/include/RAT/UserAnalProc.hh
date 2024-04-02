#ifndef __RATUserAnalProc___
#define __RATUserAnalProc___

#include <RAT/DS/Run.hh>
#include <RAT/Processor.hh>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace RAT {

class UserAnalProc : public Processor {
 public:
  UserAnalProc();
  virtual ~UserAnalProc();

  virtual Processor::Result DSEvent(DS::Root *ds) override;
  virtual void SetI(std::string param, int value) override;
  virtual void SetS(std::string param, std::string value) override;

  bool OpenFile(const std::string fileName);

 protected:
  std::string fileName_;
  TFile *outFile_;
  TTree *outTree_;
  int nPMT_;
  int verbosity_;

 protected:
  // Branch variables
  unsigned int b_Run, b_Event;
  unsigned int b_PMT_N;
  std::vector<double> b_PMT_Q, b_PMT_T;          //[pC], [ns]
  std::vector<double> b_PMT_DigiQ, b_PMT_DigiT;  // [pC],[ns]
  std::vector<double> b_PMT_NPE;                 // # of Photoelectron

  double b_MC_Edeposit, b_MC_Equench;                // [MeV]
  double b_MC_Nscint, b_MC_Nreemit, b_MC_Ncerenkov;  // # of photon

  double b_Gen_X, b_Gen_Y, b_Gen_Z, b_Gen_T;      // [mm],[mm],[mm],[ns]
  double b_Gen_Px, b_Gen_Py, b_Gen_Pz, b_Gen_KE;  // [MeV/C],[MeV/C],[MeV/C],[MeV]
  int b_Gen_PdgId;

 protected:
  // For the neutron capture analysis... to be moved in a separate sub-class
  std::vector<double> b_Ncap_GammaKE;         // [MeV]
  double b_Ncap_SumGammaKE, b_Ncap_Edeposit;  // [MeV]
  int b_Ncap_PdgId;
  std::string b_Ncap_pName;
  std::string b_Ncap_Volume;            // Volume where neutron captured , ex) "target_GdLS", "target_vessel"
  double b_Ncap_X, b_Ncap_Y, b_Ncap_Z;  // [mm],[mm],[mm]
};

}  // namespace RAT

#endif
