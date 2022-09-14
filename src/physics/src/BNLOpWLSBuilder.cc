#include <G4OpticalPhoton.hh>
#include <G4ProcessManager.hh>
#include <RAT/BNLOpWLS.hh>
#include <RAT/BNLOpWLSBuilder.hh>
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <string>

void BNLOpWLSBuilder::ConstructParticle() { G4OpticalPhoton::OpticalPhotonDefinition(); }

void BNLOpWLSBuilder::ConstructProcess() {
  BNLOpWLS *theBNLOpWLSProcess = new BNLOpWLS();

  // Pre-load the Ex/Em data
  RAT::DBLinkPtr lwls = RAT::DB::Get()->GetLink("BNL_WLS_MODEL");
  std::string matrixPath = lwls->GetS("data_path");

  std::string dataPath = static_cast<std::string>(getenv("RATSHARE")) + "/ratdb";
  if (dataPath != nullptr) {
    std::string thePath = std::string(dataPath) + "/" + matrixPath;
    theBNLOpWLSProcess->SetExEmData(thePath);
    RAT::info << "BNLOpWLSBuilder::ConstructProcess: Using data from " << thePath << newline;
  } else {
    RAT::warn << "BNLOpWLSBuilder::ConstructProcess: Warning: Could not find "
                 "Ex/Em data path for BNLOpOPWLS model"
              << newline;
  }

  if (verboseLevel > 0) {
    theBNLOpWLSProcess->DumpInfo();
  }

  theBNLOpWLSProcess->SetVerboseLevel(0);

  G4ProcessManager *pManager = 0;
  pManager = G4OpticalPhoton::OpticalPhoton()->GetProcessManager();
  if (!pManager) {
    RAT::warn << "BNLOpWLSBuilder::ConstructProcess: Error: Could not find the "
                 "Optical photon process manager"
              << newline;
    return;
  }

  theBNLOpWLSProcess->UseTimeProfile("exponential");
  pManager->AddDiscreteProcess(theBNLOpWLSProcess);
}
