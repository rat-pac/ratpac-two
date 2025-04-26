#ifndef __RATOutNtupleProc___
#define __RATOutNtupleProc___

#include <TTimeStamp.h>
#include <TTree.h>
#include <sys/types.h>

#include <RAT/DS/Run.hh>
#include <RAT/Processor.hh>
#include <functional>

#include "Math/Types.h"

class TFile;
class TTree;

namespace RAT {

class OutNtupleProc : public Processor {
 public:
  static int run_num;
  OutNtupleProc();
  virtual ~OutNtupleProc();

  enum mc_pe_type { noise = 0, cherenkov = 1, scintillation = 2, reemission = 3, unknown = 4 };

  // file - string, name of file to open for output, file will be erased
  // updatefile - string, name of file to append to
  // (do not use both file and update file)
  // virtual void SetS(std::string param, std::string value);

  // autosave - integer, update root file every N kilobytes
  // savetree 0 - Do not save the event tree.  Must set *before* file or
  // updatefile.
  // virtual void SetI(std::string param, int value);

  virtual Processor::Result DSEvent(DS::Root *ds);

  virtual bool OpenFile(std::string theFilename);

  virtual void SetI(std::string param, int value);
  virtual void SetS(std::string param, std::string value);
  // Utility function
  static ULong64_t TTimeStamp_to_UnixTime(TTimeStamp ts) {
    const ULong64_t stonano = 1000000000;
    return static_cast<ULong64_t>(ts.GetSec()) * stonano + static_cast<ULong64_t>(ts.GetNanoSec());
  }

  // Extensible functions
  virtual void AssignAdditionalAddresses(){};
  virtual void AssignAdditionalMetaAddresses(){};
  virtual void FillEvent(DS::Root *, DS::EV *){};
  virtual void FillNoTriggerEvent(DS::Root *){};
  virtual void FillMeta(){};

  // Exposed members for external tools
  DS::Run *runBranch;
  // Fill Functions
  struct NtupleOptions {
    bool tracking;
    bool mcparticles;
    bool pmthits;
    bool digitizerwaveforms;
    bool digitizerhits;
    bool digitizerfits;
    bool untriggered;
    bool mchits;
    bool nthits;
    bool calib;
  };
  NtupleOptions options;

  std::vector<std::string> waveform_fitters;
  std::map<std::string, std::vector<std::string>> waveform_fitter_FOMs;

 protected:
  std::string defaultFilename;
  TFile *outputFile;
  TTree *outputTree;
  TTree *metaTree;
  TTree *waveformTree;
  // Meta Branches
  Int_t runId;
  ULong64_t runType;
  ULong64_t runTime;
  int dsentries;
  std::string macro;
  std::vector<int> pmtType;
  std::vector<int> pmtId;
  std::vector<int> pmtChannel;
  std::vector<bool> pmtIsOnline;
  std::vector<double> pmtCableOffset;
  std::vector<double> pmtChargeScale;
  std::vector<double> pmtPulseWidthScale;
  std::vector<double> pmtX;
  std::vector<double> pmtY;
  std::vector<double> pmtZ;
  std::vector<double> pmtU;
  std::vector<double> pmtV;
  std::vector<double> pmtW;
  std::vector<int> ntId;
  std::vector<double> ntX;
  std::vector<double> ntY;
  std::vector<double> ntZ;
  std::vector<double> ntU;
  std::vector<double> ntV;
  std::vector<double> ntW;
  u_int32_t digitizerWindowSize;
  Double_t digitizerSampleRate;
  Double_t digitizerDynamicRange;
  Double_t digitizerVoltageResolution;
  // Calibration source information
  Int_t calibId;
  Int_t calibMode;
  Double_t calibIntensity;
  Double_t calibWavelength;
  std::string calibName;
  ULong64_t calibTime;
  Double_t calibX, calibY, calibZ, calibU, calibV, calibW;
  // Digitizer waveforms
  int waveform_pmtid;
  std::vector<Double_t> inWindowPulseTimes;
  std::vector<Double_t> inWindowPulseCharges;
  std::vector<UShort_t> waveform;
  // Data Branches
  Int_t mcpdg;
  double mcx, mcy, mcz;
  double mcu, mcv, mcw;
  double mcke;
  double mct;
  int evid;
  int subev;
  int nhits;
  double triggerTime;
  ULong64_t timestamp;
  ULong64_t trigger_word;
  double timeSinceLastTrigger_us;
  // MC Summary Information
  double scintEdep;
  double scintEdepQuenched;
  double scintPhotons;
  double remPhotons;
  double cherPhotons;
  // MCPMT
  int mcnhits;
  int mcpecount;
  std::vector<int> mcpmtid;
  std::vector<int> mcpmtnpe;
  std::vector<double> mcpmtcharge;
  // MCNestedTube
  int mcnNTs;
  int mcnNThits;
  std::vector<int> mcNTid;
  std::vector<double> mcNThittime;
  std::vector<double> mcNThitx;
  std::vector<double> mcNThity;
  std::vector<double> mcNThitz;
  // MCPE
  std::vector<int> mcpepmtid;
  std::vector<double> mcpehittime;
  std::vector<double> mcpefrontendtime;
  std::vector<int> mcpeprocess;
  std::vector<double> mcpewavelength;
  std::vector<double> mcpex;
  std::vector<double> mcpey;
  std::vector<double> mcpez;
  std::vector<double> mcpecharge;
  // MCParticles
  int mcpcount;
  int mcid;
  std::vector<Int_t> pdgcodes;
  std::vector<double> mcKEnergies;
  std::vector<double> mcPosx;
  std::vector<double> mcPosy;
  std::vector<double> mcPosz;
  std::vector<double> mcDirx;
  std::vector<double> mcDiry;
  std::vector<double> mcDirz;
  std::vector<double> mcTime;
  // Reconstruted variables
  std::vector<int> fitterId;
  std::vector<double> fitx, fity, fitz;
  std::vector<double> fitu, fitv, fitw;
  // Store PMT Hit Positions
  std::vector<int> hitPMTID;
  std::vector<double> hitPMTTime;
  std::vector<double> hitPMTCharge;
  // Store PMT information from digitized waveform
  int digitNhits;
  std::vector<double> digitPeak;
  std::vector<double> digitTime;
  std::vector<double> digitCharge;
  std::vector<double> digitTimeOverThreshold;
  std::vector<double> digitVoltageOverThreshold;
  std::vector<double> digitLocalTriggerTime;
  std::vector<int> digitReconNPEs;
  std::vector<int> digitNCrossings;
  std::vector<int> digitPMTID;
  // Information from fit to the waveforms
  std::map<std::string, std::vector<int>> fitPmtID;
  std::map<std::string, std::vector<double>> fitTime;
  std::map<std::string, std::vector<double>> fitCharge;
  std::map<std::string, std::map<std::string, std::vector<double>>> fitFOM;
  // std::vector<double> fitTime;
  std::vector<double> fitBaseline;
  std::vector<double> fitPeak;
  // Tracking
  std::map<std::string, int> processCodeMap;
  std::vector<int> processCodeIndex;
  std::vector<std::string> processName;
  std::map<std::string, int> volumeCodeMap;
  std::vector<int> volumeCodeIndex;
  std::vector<std::string> volumeName;

  // Tracking
  std::vector<int> trackPDG;
  std::vector<std::vector<double>> trackPosX;
  std::vector<std::vector<double>> trackPosY;
  std::vector<std::vector<double>> trackPosZ;
  std::vector<std::vector<double>> trackMomX;
  std::vector<std::vector<double>> trackMomY;
  std::vector<std::vector<double>> trackMomZ;
  std::vector<std::vector<double>> trackKE;
  std::vector<std::vector<double>> trackTime;
  std::vector<std::vector<int>> trackProcess;
  std::vector<std::vector<int>> trackVolume;

  std::set<std::string> branchNames;

  template <typename T>
  void SetBranchValue(std::string name, T *value) {
    if (branchNames.find(name) != branchNames.end()) {
      outputTree->SetBranchAddress(name.c_str(), value);
    } else {
      branchNames.insert(name);
      outputTree->Branch(name.c_str(), value);
    }
  }
};

}  // namespace RAT

#endif
