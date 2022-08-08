#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <sstream>

#include <TFile.h>
#include <TTree.h>
#include <TTimeStamp.h>
#include <TVector3.h>

#include <RAT/OutNtupleProc.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/MC.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/MCParticle.hh>
#include <RAT/DS/PathFit.hh>
#include <RAT/DS/Centroid.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DB.hh>

namespace RAT {

  OutNtupleProc::OutNtupleProc() : Processor("outntuple")
  {
    outputFile = nullptr;
    outputTree = nullptr;
    metaTree = nullptr;
    // Options

    // Load options from the database
    DB *db = DB::Get();
    DBLinkPtr table = db->GetLink("NTUPLE");
    // Setup options
  }

  bool OutNtupleProc::OpenFile(std::string filename)
  {
    int i=0;
    outputFile = TFile::Open(filename.c_str(), "RECREATE");
    // Meta Tree
    metaTree = new TTree("meta", "meta");
    metaTree->Branch("runNumber", &runNumber);
    metaTree->Branch("entries", &entries);
    metaTree->Branch("macro", &macro);
    metaTree->Branch("pmtType", &pmtType);
    metaTree->Branch("pmtId", &pmtId);
    metaTree->Branch("pmtX", &pmtX);
    metaTree->Branch("pmtY", &pmtY);
    metaTree->Branch("pmtZ", &pmtZ);
    metaTree->Branch("pmtU", &pmtU);
    metaTree->Branch("pmtV", &pmtV);
    metaTree->Branch("pmtW", &pmtW);
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
    outputTree->Branch("pdgcodes", &pdgcodes);
    outputTree->Branch("mcKEnergies", &mcKEnergies);
    outputTree->Branch("mcPosx", &mcPosx);
    outputTree->Branch("mcPosy", &mcPosy);
    outputTree->Branch("mcPosz", &mcPosz);
    outputTree->Branch("mcDirx", &mcDirx);
    outputTree->Branch("mcDiry", &mcDiry);
    outputTree->Branch("mcDirz", &mcDirz);
    outputTree->Branch("x", &x);
    outputTree->Branch("y", &y);
    outputTree->Branch("z", &z);
    outputTree->Branch("u", &u);
    outputTree->Branch("v", &v);
    outputTree->Branch("w", &w);
    outputTree->Branch("hitPMTID", &hitPMTID);
    outputTree->Branch("hitPMTTime", &hitPMTTime);
    outputTree->Branch("hitPMTCharge", &hitPMTCharge);

    return true;
  }

  Processor::Result OutNtupleProc::DSEvent(DS::Root* ds)
  {
    if(!this->outputFile){
      if(!OpenFile("test.root"))
        Log::Die("No output file specified");
    }
    ULong64_t stonano = 1000000000;
    // Clear the previous vectors
    pdgcodes.clear();
    mcKEnergies.clear();
    mcPosx.clear();
    mcPosy.clear();
    mcPosz.clear();
    mcDirx.clear();
    mcDiry.clear();
    mcDirz.clear();
    RAT::DS::MC* mc = ds->GetMC();
    TTimeStamp mcTTS = mc->GetUTC();
    ULong64_t mctime = static_cast<ULong64_t>(mcTTS.GetSec())*stonano +
                       static_cast<ULong64_t>(mcTTS.GetNanoSec());
    mcpcount = mc->GetMCParticleCount();
    for( int pid=0; pid<mcpcount; pid++ )
    {
      RAT::DS::MCParticle* particle = mc->GetMCParticle(pid);
      pdgcodes.push_back( particle->GetPDGCode() );
      mcKEnergies.push_back( particle->GetKE() );
      TVector3 mcpos = particle->GetPosition();
      TVector3 mcdir = particle->GetMomentum();
      mcPosx.push_back( mcpos.X() );
      mcPosy.push_back( mcpos.Y() );
      mcPosz.push_back( mcpos.Z() );
      mcDirx.push_back( mcdir.X()/mcdir.Mag() );
      mcDiry.push_back( mcdir.Y()/mcdir.Mag() );
      mcDirz.push_back( mcdir.Z()/mcdir.Mag() );
    }
    mcx = mcPosx[0];
    mcy = mcPosy[0];
    mcz = mcPosz[0];
    mcu = mcDirx[0];
    mcv = mcDiry[0];
    mcw = mcDirz[0];
    mcke = accumulate(mcKEnergies.begin(), mcKEnergies.end(), 0.0);
    // EV Branches
    for( subev=0; subev < ds->GetEVCount(); subev++ )
    {
      RAT::DS::EV* ev = ds->GetEV(subev);
      evid = ev->GetID();
      nanotime = static_cast<ULong64_t>(ev->GetCalibratedTriggerTime()) + mctime;
      RAT::DS::PathFit* fit = ev->GetPathFit();
      TVector3 pos = fit->GetPosition();
      x = pos.X();
      y = pos.Y();
      z = pos.Z();
      TVector3 dir = fit->GetDirection();
      u = dir.X();
      v = dir.Y();
      w = dir.Z();
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();
      for(int pmtc=0; pmtc < ev->GetPMTCount(); pmtc++)
      {
        double charge   = ev->GetPMT(pmtc)->GetCharge();
        double hit_time = ev->GetPMT(pmtc)->GetTime();
        int pmtid       = ev->GetPMT(pmtc)->GetID();
        hitPMTID.push_back(pmtid);
        hitPMTTime.push_back(hit_time);
        hitPMTCharge.push_back(charge);
      }
      // Fill
      outputTree->Fill();
    }

    // Additional branches
    //for(auto& f : this->additionalBranches){
    //  f();
    //}
    // FIX THE ABOVE
    //int errorcode = outputTree->Fill();
    //if( errorcode < 0 )
    //{
    //  Log::Die(std::string("OutNtupleProc: Error fill ttree, check disk space"));
    //}
    return Processor::OK;
  }

  OutNtupleProc::~OutNtupleProc()
  {
    if( outputFile ){
      outputFile->cd();
      //DS::RunStore::FlushWriteTree();
      metaTree->Write();
      outputTree->Write();
      /*
      TObjString* macro = new TObjString(Log::GetMacro().c_str());
      macro->Write("macro");

      TMap* dbtrace = Log::GetDBTraceMap();
      dbtrace->Write("db", TObject::kSingleKey);
      */
      outputFile->Write(0, TObject::kOverwrite);
      outputFile->Close();
      delete outputFile;
    }
  }
}
