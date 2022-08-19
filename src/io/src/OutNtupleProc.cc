#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <TFile.h>
#include <TTimeStamp.h>
#include <TTree.h>
#include <TVector3.h>

#include <RAT/DB.hh>
#include <RAT/DS/Centroid.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/MC.hh>
#include <RAT/DS/MCParticle.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/PathFit.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/OutNtupleProc.hh>

namespace RAT {

OutNtupleProc::OutNtupleProc() : Processor("outntuple") {
  outputFile = nullptr;
  outputTree = nullptr;
  metaTree = nullptr;
  runBranch = new DS::Run();

  // Load options from the database
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("IO", "NtupleProc");
  try {
    defaultFilename = table->GetS("default_output_filename");
    if (defaultFilename.find(".") == std::string::npos) {
      defaultFilename += ".ntuple.root";
    }
  } catch (DBNotFoundError &e) {
    defaultFilename = "output.ntuple.root";
  }
  try {
    options.tracking = table->GetZ("include_tracking");
    options.mcparticles = table->GetZ("include_mcparticles");
    options.pmthits = table->GetZ("include_pmthits");
    options.untriggered = table->GetZ("include_untriggered_events");
  } catch (DBNotFoundError &e) {
    options.tracking = false;
    options.mcparticles = false;
    options.pmthits = true;
    options.untriggered = false;
  }
}

bool OutNtupleProc::OpenFile(std::string filename) {
  int i = 0;
  outputFile = TFile::Open(filename.c_str(), "RECREATE");
  // Meta Tree
  metaTree = new TTree("meta", "meta");
  metaTree->Branch("runId", &runId);
  metaTree->Branch("runType", &runType);
  metaTree->Branch("runTime", &runTime);
  metaTree->Branch("dsentries", &dsentries);
  metaTree->Branch("macro", &macro);
  metaTree->Branch("pmtType", &pmtType);
  metaTree->Branch("pmtId", &pmtId);
  metaTree->Branch("pmtX", &pmtX);
  metaTree->Branch("pmtY", &pmtY);
  metaTree->Branch("pmtZ", &pmtZ);
  metaTree->Branch("pmtU", &pmtU);
  metaTree->Branch("pmtV", &pmtV);
  metaTree->Branch("pmtW", &pmtW);
  dsentries = 0;
  // Data Tree
  outputTree = new TTree("output", "output");
  outputTree->Branch("mcx", &mcx);
  outputTree->Branch("mcy", &mcy);
  outputTree->Branch("mcz", &mcz);
  outputTree->Branch("mcu", &mcu);
  outputTree->Branch("mcv", &mcv);
  outputTree->Branch("mcw", &mcw);
  outputTree->Branch("mcke", &mcke);
  outputTree->Branch("evid", &evid);
  outputTree->Branch("subev", &subev);
  outputTree->Branch("nanotime", &nanotime);
  outputTree->Branch("mcpcount", &mcpcount);
  if (options.mcparticles) {
    outputTree->Branch("pdgcodes", &pdgcodes);
    outputTree->Branch("mcKEnergies", &mcKEnergies);
    outputTree->Branch("mcPosx", &mcPosx);
    outputTree->Branch("mcPosy", &mcPosy);
    outputTree->Branch("mcPosz", &mcPosz);
    outputTree->Branch("mcDirx", &mcDirx);
    outputTree->Branch("mcDiry", &mcDiry);
    outputTree->Branch("mcDirz", &mcDirz);
  }
  // outputTree->Branch("fitx", &fitx);
  // outputTree->Branch("fity", &fity);
  // outputTree->Branch("fitz", &fitz);
  // outputTree->Branch("fitu", &fitu);
  // outputTree->Branch("fitv", &fitv);
  // outputTree->Branch("fitw", &fitw);
  if (options.pmthits) {
    outputTree->Branch("hitPMTID", &hitPMTID);
    outputTree->Branch("hitPMTTime", &hitPMTTime);
    outputTree->Branch("hitPMTCharge", &hitPMTCharge);
  }
  if (options.tracking) {
    outputTree->Branch("trackPDG", &trackPDG);
    outputTree->Branch("trackPosX", &trackPosX);
    outputTree->Branch("trackPosY", &trackPosY);
    outputTree->Branch("trackPosZ", &trackPosZ);
    outputTree->Branch("trackMomX", &trackMomX);
    outputTree->Branch("trackMomY", &trackMomY);
    outputTree->Branch("trackMomZ", &trackMomZ);
    outputTree->Branch("trackKE", &trackKE);
    outputTree->Branch("trackTime", &trackTime);
    outputTree->Branch("trackProcess", &trackProcess);
    metaTree->Branch("processCodeMap", &processCodeMap);
  }

  return true;
}

Processor::Result OutNtupleProc::DSEvent(DS::Root *ds) {
  if (!this->outputFile) {
    if (!OpenFile(this->defaultFilename.c_str())) {
      Log::Die("No output file specified");
    }
  }
  runBranch = RAT::DS::RunStore::GetRun(ds);
  ULong64_t stonano = 1000000000;
  dsentries++;
  // Clear the previous vectors
  pdgcodes.clear();
  mcKEnergies.clear();
  mcPosx.clear();
  mcPosy.clear();
  mcPosz.clear();
  mcDirx.clear();
  mcDiry.clear();
  mcDirz.clear();

  RAT::DS::MC *mc = ds->GetMC();
  TTimeStamp mcTTS = mc->GetUTC();
  ULong64_t mctime = static_cast<ULong64_t>(mcTTS.GetSec()) * stonano +
                     static_cast<ULong64_t>(mcTTS.GetNanoSec());
  mcpcount = mc->GetMCParticleCount();
  for (int pid = 0; pid < mcpcount; pid++) {
    RAT::DS::MCParticle *particle = mc->GetMCParticle(pid);
    pdgcodes.push_back(particle->GetPDGCode());
    mcKEnergies.push_back(particle->GetKE());
    TVector3 mcpos = particle->GetPosition();
    TVector3 mcdir = particle->GetMomentum();
    mcPosx.push_back(mcpos.X());
    mcPosy.push_back(mcpos.Y());
    mcPosz.push_back(mcpos.Z());
    mcDirx.push_back(mcdir.X() / mcdir.Mag());
    mcDiry.push_back(mcdir.Y() / mcdir.Mag());
    mcDirz.push_back(mcdir.Z() / mcdir.Mag());
  }
  mcx = mcPosx[0];
  mcy = mcPosy[0];
  mcz = mcPosz[0];
  mcu = mcDirx[0];
  mcv = mcDiry[0];
  mcw = mcDirz[0];
  mcke = accumulate(mcKEnergies.begin(), mcKEnergies.end(), 0.0);
  // Tracking
  if (options.tracking) {
    int nTracks = mc->GetMCTrackCount();
    // Clear previous event
    trackPDG.clear();
    trackPosX.clear();
    trackPosY.clear();
    trackPosZ.clear();
    trackMomX.clear();
    trackMomY.clear();
    trackMomZ.clear();
    trackKE.clear();
    trackTime.clear();
    trackProcess.clear();

    std::vector<double> xtrack, ytrack, ztrack;
    std::vector<double> pxtrack, pytrack, pztrack;
    std::vector<double> kinetic, localtime;
    std::vector<int> processMapID;
    for (int trk = 0; trk < nTracks; trk++) {
      RAT::DS::MCTrack *track = mc->GetMCTrack(trk);
      trackPDG.push_back(track->GetPDGCode());
      xtrack.clear();
      ytrack.clear();
      ztrack.clear();
      pxtrack.clear();
      pytrack.clear();
      pztrack.clear();
      kinetic.clear();
      localtime.clear();
      processMapID.clear();
      int nSteps = track->GetMCTrackStepCount();
      for (int stp = 0; stp < nSteps; stp++) {
        RAT::DS::MCTrackStep *step = track->GetMCTrackStep(stp);
        // Process
        std::string proc = step->GetProcess();
        if (processCodeMap.find(proc) == processCodeMap.end()) {
          processCodeMap[proc] = processCodeMap.size();
          processCodeIndex.push_back(processCodeMap.size() - 1);
          processName.push_back(proc);
        }
        processMapID.push_back(processCodeMap[proc]);
        TVector3 tv = step->GetEndpoint();
        TVector3 momentum = step->GetMomentum();
        kinetic.push_back(step->GetKE());
        localtime.push_back(step->GetLocalTime());
        xtrack.push_back(tv.X());
        ytrack.push_back(tv.Y());
        ztrack.push_back(tv.Z());
        pxtrack.push_back(momentum.X());
        pytrack.push_back(momentum.Y());
        pztrack.push_back(momentum.Z());
      }
      trackKE.push_back(kinetic);
      trackTime.push_back(localtime);
      trackPosX.push_back(xtrack);
      trackPosY.push_back(ytrack);
      trackPosZ.push_back(ztrack);
      trackMomX.push_back(pxtrack);
      trackMomY.push_back(pytrack);
      trackMomZ.push_back(pztrack);
      trackProcess.push_back(processMapID);
    }
  }
  // EV Branches
  for (subev = 0; subev < ds->GetEVCount(); subev++) {
    RAT::DS::EV *ev = ds->GetEV(subev);
    evid = ev->GetID();
    nanotime = static_cast<ULong64_t>(ev->GetCalibratedTriggerTime()) + mctime;
    // Todo: Need to decide how to add fit information
    // RAT::DS::PathFit *fit = ev->GetPathFit();
    // TVector3 pos = fit->GetPosition();
    // fitx = pos.X();
    // fity = pos.Y();
    // fitz = pos.Z();
    // TVector3 dir = fit->GetDirection();
    // fitu = dir.X();
    // fitv = dir.Y();
    // fitw = dir.Z();

    if (options.pmthits) {
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();
      for (int pmtc = 0; pmtc < ev->GetPMTCount(); pmtc++) {
        double charge = ev->GetPMT(pmtc)->GetCharge();
        double hit_time = ev->GetPMT(pmtc)->GetTime();
        int pmtid = ev->GetPMT(pmtc)->GetID();
        hitPMTID.push_back(pmtid);
        hitPMTTime.push_back(hit_time);
        hitPMTCharge.push_back(charge);
      }
    }
    // Fill
    outputTree->Fill();
  }
  if (options.untriggered && ds->GetEVCount() == 0) {
    evid = 0;
    nanotime = mctime;
    // fitxx = -999999;
    // fitxy = -999999;
    // fitxz = -999999;
    // fitxu = 0;
    // fitxv = 0;
    // fitxw = 0;
    if (options.pmthits) {
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();
    }
    outputTree->Fill();
  }

  // Additional branches
  // for(auto& f : this->additionalBranches){
  //  f();
  //}
  // FIX THE ABOVE
  // int errorcode = outputTree->Fill();
  // if( errorcode < 0 )
  //{
  //  Log::Die(std::string("OutNtupleProc: Error fill ttree, check disk
  //  space"));
  //}
  return Processor::OK;
}

OutNtupleProc::~OutNtupleProc() {
  if (outputFile) {
    outputFile->cd();
    RAT::DS::PMTInfo *pmtinfo = runBranch->GetPMTInfo();
    for (int id = 0; id < pmtinfo->GetPMTCount(); id++) {
      int type = pmtinfo->GetType(id);
      TVector3 position = pmtinfo->GetPosition(id);
      TVector3 direction = pmtinfo->GetDirection(id);
      pmtType.push_back(type);
      pmtId.push_back(id);
      pmtX.push_back(position.X());
      pmtY.push_back(position.Y());
      pmtZ.push_back(position.Z());
      pmtU.push_back(direction.X());
      pmtV.push_back(direction.Y());
      pmtW.push_back(direction.Z());
    }
    runId = runBranch->GetID();
    runType = runBranch->GetType();
    // Converting to unix time
    ULong64_t stonano = 1000000000;
    TTimeStamp rootTime = runBranch->GetStartTime();
    runTime = static_cast<ULong64_t>(rootTime.GetSec()) * stonano +
              static_cast<ULong64_t>(rootTime.GetNanoSec());
    macro = Log::GetMacro();
    metaTree->Fill();
    metaTree->Write();
    outputTree->Write();
    /*
    TMap* dbtrace = Log::GetDBTraceMap();
    dbtrace->Write("db", TObject::kSingleKey);
    */
    //outputFile->Write(0, TObject::kOverwrite);
    outputFile->Close();
    delete outputFile;
  }
}

void OutNtupleProc::SetS(std::string param, std::string value) {
  if (param == "file") {
    this->defaultFilename = value;
  }
}

void OutNtupleProc::SetI(std::string param, int value) {
  if (param == "include_tracking") {
    options.tracking = value ? true : false;
  }
  if (param == "include_mcparticles") {
    options.mcparticles = value ? true : false;
  }
  if (param == "include_pmthits") {
    options.pmthits = value ? true : false;
  }
  if (param == "include_untriggered_events") {
    options.untriggered = value ? true : false;
  }
}
} // namespace RAT
