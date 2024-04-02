#include "RAT/UserAnalProc.hh"

#include <TFile.h>
#include <TTree.h>

#include <iostream>

using namespace std;

namespace RAT {

UserAnalProc::UserAnalProc() : Processor("Analntuple") {
  fileName_ = "ntuple.root";
  outFile_ = nullptr;
  outTree_ = nullptr;

  nPMT_ = 0;

  verbosity_ = 0;
}

UserAnalProc::~UserAnalProc() {
  if (!outFile_) return;

  outFile_->cd();
  outTree_->Write();
  outFile_->Close();
}

void UserAnalProc::SetS(std::string param, std::string value) {
  if (param == "file") {
    fileName_ = value;
  }
}

void UserAnalProc::SetI(std::string param, int value) {
  if (param == "nPMT") {
    b_PMT_N = nPMT_ = value;
  } else if (param == "verbosity") {
    verbosity_ = value;
  }
}

bool UserAnalProc::OpenFile(const std::string fileName) {
  outFile_ = TFile::Open(fileName.c_str(), "RECREATE");
  if (!outFile_ or !outFile_->IsOpen()) return false;
  outFile_->cd();

  // Data Tree
  outTree_ = new TTree("output", "output");

  outTree_->Branch("Run", &b_Run, "Run/i");
  outTree_->Branch("Event", &b_Event, "Event/i");

  b_PMT_NPE.resize(nPMT_);
  b_PMT_Q.resize(nPMT_);
  b_PMT_T.resize(nPMT_);
  b_PMT_DigiQ.resize(nPMT_);
  b_PMT_DigiT.resize(nPMT_);
  outTree_->Branch("PMT_N", &b_PMT_N, "PMT_N/i");
  outTree_->Branch("PMT_NPE", &b_PMT_NPE[0], "PMT_NPE[PMT_N]/D");
  outTree_->Branch("PMT_Q", &b_PMT_Q[0], "PMT_Q[PMT_N]/D");
  outTree_->Branch("PMT_T", &b_PMT_T[0], "PMT_T[PMT_N]/D");
  outTree_->Branch("PMT_DigiQ", &b_PMT_DigiQ[0], "PMT_DigiQ[PMT_N]/D");
  outTree_->Branch("PMT_DigiT", &b_PMT_DigiT[0], "PMT_DigiT[PMT_N]/D");

  outTree_->Branch("MC_Edeposit", &b_MC_Edeposit, "MC_Edeposit/D");
  outTree_->Branch("MC_Equench", &b_MC_Equench, "MC_Equench/D");
  outTree_->Branch("MC_Nscint", &b_MC_Nscint, "MC_nscint/D");
  outTree_->Branch("MC_Neeemit", &b_MC_Nreemit, "MC_Nreemit/D");
  outTree_->Branch("MC_Ncerenkov", &b_MC_Ncerenkov, "MC_Ncerenkov/D");

  outTree_->Branch("Gen_X", &b_Gen_X, "Gen_X/D");
  outTree_->Branch("Gen_Y", &b_Gen_Y, "Gen_Y/D");
  outTree_->Branch("Gen_Z", &b_Gen_Z, "Gen_Z/D");
  outTree_->Branch("Gen_T", &b_Gen_T, "Gen_T/D");

  outTree_->Branch("Gen_Px", &b_Gen_Px, "Gen_Px/D");
  outTree_->Branch("Gen_Py", &b_Gen_Py, "Gen_Py/D");
  outTree_->Branch("Gen_Pz", &b_Gen_Pz, "Gen_Pz/D");
  outTree_->Branch("Gen_KE", &b_Gen_KE, "Gen_KE/D");

  outTree_->Branch("Gen_PdgId", &b_Gen_PdgId, "Gen_PdgId/I");

  // Variables for neutron capture analysis
  outTree_->Branch("Ncap_GammaKE", &b_Ncap_GammaKE);
  outTree_->Branch("Ncap_SumGammaKE", &b_Ncap_SumGammaKE, "Ncap_SumGammaKE/D");
  outTree_->Branch("Ncap_PdgId", &b_Ncap_PdgId, "Ncap_PdgId/I");
  outTree_->Branch("Ncap_pName", &b_Ncap_pName);
  outTree_->Branch("Ncap_Edeposit", &b_Ncap_Edeposit, "Ncap_Edeposit/D");
  outTree_->Branch("Ncap_Volume", &b_Ncap_Volume);
  outTree_->Branch("Ncap_X", &b_Ncap_X, "Ncap_X/D");
  outTree_->Branch("Ncap_Y", &b_Ncap_Y, "Ncap_Y/D");
  outTree_->Branch("Ncap_Z", &b_Ncap_Z, "Ncap_Z/D");

  return true;
}

Processor::Result UserAnalProc::DSEvent(DS::Root *ds) {
  if (!outFile_) {
    if (!OpenFile(fileName_)) return Processor::FAIL;
  }

  DS::MC *mcEvent = ds->GetMC();
  if (!mcEvent) return Processor::FAIL;
  b_Run = ds->GetRunID();
  b_Event = mcEvent->GetID();

  const int nPMT = mcEvent->GetMCPMTCount();
  if (nPMT > nPMT_) {
    if (verbosity_ > 0) cerr << "nPMT in mcEvent > nPMT in the setup!!!" << endl;
    return Processor::ABORT;
  }

  for (int i = 0; i < nPMT_; ++i) {
    b_PMT_Q[i] = b_PMT_T[i] = b_PMT_NPE[i] = 0;
    b_PMT_DigiQ[i] = b_PMT_DigiT[i] = 0;
  }
  b_MC_Edeposit = b_MC_Equench = 0;
  b_MC_Nscint = b_MC_Nreemit = b_MC_Ncerenkov = 0;
  b_Gen_X = b_Gen_Y = b_Gen_Z = 0;
  b_Gen_Px = b_Gen_Py = b_Gen_Pz = b_Gen_KE = 0;
  b_Gen_PdgId = 0;

  b_Ncap_GammaKE.clear();
  b_Ncap_SumGammaKE = b_Ncap_Edeposit = 0;
  b_Ncap_PdgId = 0;
  b_Ncap_pName = "";
  b_Ncap_Volume = "";
  b_Ncap_X = b_Ncap_Y = b_Ncap_Z = 0;

  for (int subev = 0; subev < ds->GetEVCount(); ++subev) {
    auto event = ds->GetEV(subev);

    for (int i = 0, n = event->GetPMTCount(); i < n; ++i) {
      const auto pmt = event->GetPMT(i);
      const int iPMT = pmt->GetID();
      if (iPMT >= nPMT_) continue;

      b_PMT_Q[iPMT] = pmt->GetCharge();
      b_PMT_T[iPMT] = pmt->GetTime();
      b_PMT_DigiT[iPMT] = pmt->GetDigitizedTime();
      b_PMT_DigiQ[iPMT] = pmt->GetDigitizedCharge();
    }
  }

  const std::set<std::string> particleNames = {"deuteron", "Gd155", "Gd156", "Gd157", "Gd158",
                                               "Gd159",    "Gd160", "C12",   "C13"};

  auto track_Ncap = mcEvent->GetMCTrack(0);
  const std::string pName_Ncap = track_Ncap->GetParticleName();
  auto step_Ncap = track_Ncap->GetMCTrackStep(track_Ncap->GetMCTrackStepCount() - 1);
  b_Ncap_Volume = step_Ncap->GetVolume();

  auto endPoint_Ncap = step_Ncap->GetEndpoint();
  b_Ncap_X = endPoint_Ncap.X();
  b_Ncap_Y = endPoint_Ncap.Y();
  b_Ncap_Z = endPoint_Ncap.Z();

  if (verbosity_ > 0) cout << "mcEvent->GetMCTrackCount() " << mcEvent->GetMCTrackCount() << endl;
  for (int iTrack = 0, nTrack = mcEvent->GetMCTrackCount(); iTrack < nTrack; ++iTrack) {
    auto track = mcEvent->GetMCTrack(iTrack);
    const std::string pName = track->GetParticleName();
    if (pName == "opticalphoton") continue;
    // if ( pName != "gamma" ) continue;
    double sumKE = 0;
    double sumEdepot = 0;

    for (int iStep = 0, nStep = track->GetMCTrackStepCount(); iStep < nStep; ++iStep) {
      auto step = track->GetMCTrackStep(iStep);
      if (step->GetProcess() != "nCapture") continue;
      const std::string stepPName = track->GetParticleName();
      if (particleNames.find(pName) != particleNames.end()) {
        b_Ncap_PdgId = track->GetPDGCode();
        b_Ncap_pName = track->GetParticleName();
      }

      if (pName == "gamma") b_Ncap_GammaKE.push_back(step->GetKE());
      if (pName == "gamma" || pName == "e-") sumKE += step->GetKE();
      sumEdepot += step->GetDepositedEnergy();
      // const int stepPdgId = track->GetPDGCode();
      if (verbosity_ > 0)
        cout << iTrack << ' ' << iStep << ' ' << pName << ' ' << stepPName << ' ' << step->GetKE() << endl;
    }

    b_Ncap_SumGammaKE += sumKE;
    b_Ncap_Edeposit += sumEdepot;
  }

  const auto mcSummary = mcEvent->GetMCSummary();
  const double energyDep = mcSummary->GetTotalScintEdep();
  b_MC_Edeposit = energyDep;

  const double energyDep_quenched = mcSummary->GetTotalScintEdepQuenched();
  b_MC_Equench = energyDep_quenched;

  b_MC_Nscint = mcSummary->GetNumScintPhoton();
  b_MC_Nreemit = mcSummary->GetNumReemitPhoton();
  b_MC_Ncerenkov = mcSummary->GetNumCerenkovPhoton();

  const auto mcParticle = mcEvent->GetMCParticle(0);
  b_Gen_PdgId = mcParticle->GetPDGCode();

  const auto particlePos = mcParticle->GetPosition();
  b_Gen_X = particlePos.X();
  b_Gen_Y = particlePos.Y();
  b_Gen_Z = particlePos.Z();
  b_Gen_T = mcParticle->GetTime();

  const auto momentum = mcParticle->GetMomentum();
  b_Gen_Px = momentum.X();
  b_Gen_Py = momentum.Y();
  b_Gen_Pz = momentum.Z();
  b_Gen_KE = mcParticle->GetKE();

  const int nMCPMT = mcEvent->GetMCPMTCount();
  for (int iPMT = 0; iPMT < nMCPMT; ++iPMT) {
    auto mcPMT = mcEvent->GetMCPMT(iPMT);
    const int pmtID = mcPMT->GetID();
    if (iPMT >= nPMT_) {
      if (verbosity_ > 0) cerr << "!!! Wrong PMT ID number " << iPMT << " >= " << nPMT_ << ". Skip this PMT\n";
      continue;
    }
    const int nHit = mcPMT->GetMCPhotonCount();
    b_PMT_NPE[pmtID] += nHit;
  }

  outTree_->Fill();

  return Processor::OK;
}

}  // namespace RAT
