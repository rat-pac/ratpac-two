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
#include <RAT/OutNtupleProc.hh>

namespace RAT {

OutNtupleProc::OutNtupleProc() : Processor("outntuple") {
  outputFile = nullptr;
  outputTree = nullptr;
  metaTree = nullptr;
  runBranch = new DS::Run();
  // Options

  // Load options from the database
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("IO","NtupleProc");
  try {
    defaultFilename = table->GetS("default_output_filename");
    if( defaultFilename.find(".") == std::string::npos ){
      defaultFilename += ".ntuple.root";
    }
  } catch (DBNotFoundError &e) {
    defaultFilename = "output.ntuple.root";
  }
  options.tracking = table->GetZ("include_tracking");
  options.mcparticles = table->GetZ("include_mcparticles");
  options.pmthits = table->GetZ("include_pmthits");
  options.untriggered = table->GetZ("include_untriggered_events");
}

bool OutNtupleProc::OpenFile(std::string filename) {
  int i = 0;
  outputFile = TFile::Open(filename.c_str(), "RECREATE");
  // Meta Tree
  metaTree = new TTree("meta", "meta");
  //metaTree->Branch("runId", &runId);
  //metaTree->Branch("runType", &runType);
  //metaTree->Branch("runTime", &runTime);
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
  if( options.mcparticles ) {
    outputTree->Branch("pdgcodes", &pdgcodes);
    outputTree->Branch("mcKEnergies", &mcKEnergies);
    outputTree->Branch("mcPosx", &mcPosx);
    outputTree->Branch("mcPosy", &mcPosy);
    outputTree->Branch("mcPosz", &mcPosz);
    outputTree->Branch("mcDirx", &mcDirx);
    outputTree->Branch("mcDiry", &mcDiry);
    outputTree->Branch("mcDirz", &mcDirz);
  }
  outputTree->Branch("x", &x);
  outputTree->Branch("y", &y);
  outputTree->Branch("z", &z);
  outputTree->Branch("u", &u);
  outputTree->Branch("v", &v);
  outputTree->Branch("w", &w);
  if( options.pmthits ) {
    outputTree->Branch("hitPMTID", &hitPMTID);
    outputTree->Branch("hitPMTTime", &hitPMTTime);
    outputTree->Branch("hitPMTCharge", &hitPMTCharge);
  }

  return true;
}

Processor::Result OutNtupleProc::DSEvent(DS::Root *ds) {
  if (!this->outputFile) {
    if (!OpenFile(this->defaultFilename.c_str())) {
      Log::Die("No output file specified");
    }
  }
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
  // EV Branches
  for (subev = 0; subev < ds->GetEVCount(); subev++) {
    RAT::DS::EV *ev = ds->GetEV(subev);
    evid = ev->GetID();
    nanotime = static_cast<ULong64_t>(ev->GetCalibratedTriggerTime()) + mctime;
    RAT::DS::PathFit *fit = ev->GetPathFit();
    TVector3 pos = fit->GetPosition();
    x = pos.X();
    y = pos.Y();
    z = pos.Z();
    TVector3 dir = fit->GetDirection();
    u = dir.X();
    v = dir.Y();
    w = dir.Z();

    if( options.pmthits ) {
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
    if( options.tracking ){
    }
    // Fill
    outputTree->Fill();
  }
  if( options.untriggered && ds->GetEVCount() == 0 ) {
    evid = 0;
    nanotime = mctime;
    x = -999999;
    y = -999999;
    z = -999999;
    u = 0;
    v = 0;
    w = 0;
    if( options.pmthits ) {
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();
    }
    if( options.tracking ) {
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
    RAT::DS::PMTInfo* pmtinfo = runBranch->GetPMTInfo();
    for( int id=0; id < pmtinfo->GetPMTCount(); id++ ){
      int type = pmtinfo->GetType(id);
      TVector3 position = pmtinfo->GetPosition(id);
      TVector3 direction = pmtinfo->GetDirection(id);
      pmtType.push_back( type );
      pmtId.push_back( id );
      pmtX.push_back( position.X() );
      pmtY.push_back( position.Y() );
      pmtZ.push_back( position.Z() );
      pmtU.push_back( direction.X() );
      pmtV.push_back( direction.Y() );
      pmtW.push_back( direction.Z() );
    }
    runId = runBranch->GetID();
    runType = runBranch->GetType();
    runTime = runBranch->GetStartTime();
    macro = Log::GetMacro();
    metaTree->Fill();
    // DS::RunStore::FlushWriteTree();
    metaTree->Write();
    outputTree->Write();
    /*
    TMap* dbtrace = Log::GetDBTraceMap();
    dbtrace->Write("db", TObject::kSingleKey);
    */
    outputFile->Write(0, TObject::kOverwrite);
    outputFile->Close();
    delete outputFile;
  }
}

void OutNtupleProc::SetS(std::string param, std::string value){
  if(param == "file"){
    this->defaultFilename = value;
  }
}

void OutNtupleProc::SetZ(std::string param, bool value){
  if( param == "include_tracking" ){
    options.tracking = value;
  }
  if( param == "include_mcparticles" ){
    options.mcparticles = value;
  }
  if( param == "include_pmthits" ){
    options.pmthits= value;
  }
  if( param == "include_untriggered_events" ){
    options.untriggered = value;
  }
}
} // namespace RAT
