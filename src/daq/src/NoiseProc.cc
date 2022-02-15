#include <vector>
#include <algorithm>
#include <map>
#include <Randomize.hh>
#include <RAT/NoiseProc.hh>
#include <RAT/DB.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/PDFPMTTime.hh>
#include <RAT/PDFPMTCharge.hh>
#include <RAT/MiniCleanPMTCharge.hh>

using namespace std;

namespace RAT {

NoiseProc::NoiseProc() : Processor("noise") {
  DBLinkPtr lnoise = DB::Get()->GetLink("NOISEPROC");

  fNoiseRate   = lnoise->GetD("noise_rate");
  fLookback    = lnoise->GetD("noise_lookback");
  fLookforward = lnoise->GetD("noise_lookforward");
  fMaxTime     = lnoise->GetD("noise_maxtime");
  fNearHits    = lnoise->GetI("noise_nearhits");
}

Processor::Result NoiseProc::DSEvent(DS::Root* ds) {
  // Noise moved to a processor from GSim, this is a special
  // case of a processor modifying the MC branch

  // Run Information
  DS::Run* run         = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo* pmtinfo = run->GetPMTInfo();

  // By the way, this is a cheat. Once we have BeginOfRunActions
  // in processors, this gets moved there.
  UpdatePMTModels(pmtinfo);

  // Write over MC
  DS::MC* mc = ds->GetMC();
  // Loop through current hits to get a full window and hit pmts
  double firsthittime = std::numeric_limits<double>::max();
  double lasthittime  = std::numeric_limits<double>::min();

  // <pmtid, mcpmtlocation>
  std::map<int, int> mcpmtObjects;

  // <pmt hit times>
  std::vector<double> realHitTimes;

  for(int imcpmt=0; imcpmt < mc->GetMCPMTCount(); imcpmt++)
  {
    DS::MCPMT* mcpmt = mc->GetMCPMT(imcpmt);
    mcpmtObjects[mcpmt->GetID()] = imcpmt;
    for(int pidx=0; pidx < mcpmt->GetMCPhotonCount(); pidx++)
    {
      DS::MCPhoton* photon = mcpmt->GetMCPhoton(pidx);
      if( photon->IsDarkHit() ) 
        mcpmt->RemoveMCPhoton(pidx);
      else
      {
        double hittime = photon->GetHitTime();
        if( hittime < firsthittime ) firsthittime = hittime;
        if( hittime > lasthittime  ) lasthittime  = hittime;
        realHitTimes.push_back(hittime);
      }
    }
  }

  // Cap how far forward to look in case of weird geant-4 lifetimes
  if( lasthittime > fMaxTime ) lasthittime = fMaxTime;
  // And really just in case
  if( firsthittime < -fMaxTime ) firsthittime = -fMaxTime;
  double noiseBegin       = firsthittime - fLookback;
  double noiseEnd         = lasthittime + fLookforward;

  // Look around real hits or everywhere?
  int totalNoiseHits = 0;
  if( fNearHits )
  {
    std::map<double, double> windowMap = FindWindows(realHitTimes, max(abs(fLookback), abs(fLookforward)));
    if( !windowMap.empty() )
    {
      for( auto m : windowMap )
      {
        totalNoiseHits += GenerateNoiseInWindow(mc, m.first - fLookback, m.second + fLookforward, pmtinfo, mcpmtObjects);
      }
    }
  }
  else
  {
    totalNoiseHits += GenerateNoiseInWindow(mc, noiseBegin, noiseEnd, pmtinfo, mcpmtObjects);
  }
  mc->SetNumDark( totalNoiseHits );

  return Processor::OK;
}

int NoiseProc::GenerateNoiseInWindow( DS::MC* mc, double noiseBegin, 
    double noiseEnd, DS::PMTInfo* pmtinfo, std::map<int, int> mcpmtObjects )
{
  double noiseWindowWidth = noiseEnd - noiseBegin;
  size_t pmtCount         = pmtinfo->GetPMTCount();
  double darkCount        = fNoiseRate * noiseWindowWidth * pmtCount * 1e-9;

  int noiseHits = static_cast<int>(floor(CLHEP::RandPoisson::shoot(darkCount)));
  for(int ihit=0; ihit < noiseHits; ihit++)
  {
    int pmtid = static_cast<int>(G4UniformRand() * pmtCount);
    if( !mcpmtObjects.count(pmtid) )
    {
      DS::MCPMT* mcpmt = mc->AddNewMCPMT();
      mcpmtObjects[pmtid] = mc->GetMCPMTCount() - 1;
      mcpmt->SetID(pmtid);
      mcpmt->SetType( pmtinfo->GetType(pmtid) );
    }
    DS::MCPMT* mcpmt = mc->GetMCPMT( mcpmtObjects[pmtid] );
    double hittime = noiseBegin + G4UniformRand()*noiseWindowWidth;
    AddNoiseHit( mcpmt, pmtinfo, hittime );
  }
  return noiseHits;
}

void NoiseProc::AddNoiseHit( DS::MCPMT* mcpmt, DS::PMTInfo* pmtinfo, 
    double hittime )
{
    DS::MCPhoton* photon = mcpmt->AddNewMCPhoton();
    photon->SetDarkHit(true);
    photon->SetLambda(0.0);
    photon->SetPosition(TVector3(0, 0, 0));
    photon->SetMomentum(TVector3(0, 0, 0));
    photon->SetPolarization(TVector3(0, 0, 0));
    photon->SetTrackID(-1);
    photon->SetHitTime( hittime );
    // Modify these to check the pmt time and charge model

    photon->SetFrontEndTime(
        fPMTTime[ pmtinfo->GetModel(mcpmt->GetID()) ]->PickTime( hittime )
        );
    photon->SetCharge(
        fPMTCharge[ pmtinfo->GetModel(mcpmt->GetID()) ]->PickCharge()
        );
    mcpmt->SortMCPhotons();

    return;
}

void NoiseProc::UpdatePMTModels( DS::PMTInfo* pmtinfo )
{
  // This is a bit hacky, but we don't want to keep
  // running this every event, so after the first time it
  // is skipped.
  if( fPMTTime.size() > 0 ) return;
  const size_t numModels = pmtinfo->GetModelCount();
  fPMTTime.resize(numModels);
  fPMTCharge.resize(numModels);
  for(size_t i=0; i<numModels; i++)
  {
    const std::string modelName = pmtinfo->GetModelName(i);
    try{
      fPMTTime[i] = new RAT::PDFPMTTime(modelName);
    }
    catch (DBNotFoundError& e){
      fPMTTime[i] = new RAT::PDFPMTTime();
    }
    try{
      fPMTCharge[i] = new RAT::PDFPMTCharge(modelName);
    }
    catch (DBNotFoundError& e){
      fPMTCharge[i] = new RAT::MiniCleanPMTCharge();
    }
  }
  return;
}

std::map<double, double> NoiseProc::FindWindows(std::vector<double> &times, double window)
{
  // Sort the hit times
  std::sort(times.begin(), times.end());
  // Find the time differences along the times array
  std::map<double, double> startStop;
  // If there are too few hits, generate noise near trigger??
  if( times.size() < 2 )
  {
    if( times.size() == 0 )
      return startStop;
    else
      startStop[times[0]] = times[0];
    return startStop;
  }
  vector<double>::iterator back = times.begin();
  double start = *back;
  double stop = 0;
  for( vector<double>::iterator forward = next(times.begin());
       forward < times.end(); ++forward )
  {
    if( (*forward - *back) > window )
    {
      stop = *back;
      startStop[start] = stop;
      start = *forward;
    }
    ++back;
  }
  // Grab last hit window
  stop = *back;
  startStop[start] = stop;
  return startStop;
}

void NoiseProc::SetD(std::string param, double value)
{
  if (param == "rate")
    if(value > 0)
      fNoiseRate = value;
    else
      throw ParamInvalid(param, "Noise rate must be positive");
  else if(param == "lookback")
    fLookback = abs(value);
  else if(param == "lookforward")
    fLookforward = abs(value);
  else if(param == "maxtime")
    fMaxTime = abs(value);
  else
    throw ParamUnknown(param);
}

void NoiseProc::SetI(std::string param, int value)
{
  if (param == "nearhits")
    fNearHits = value;
  else
    throw ParamUnknown(param);
}

} // namespace RAT
