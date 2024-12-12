#include <RAT/DB.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/NoiseProc.hh>
#include <RAT/PDFPMTCharge.hh>
#include <RAT/PDFPMTTime.hh>
#include <Randomize.hh>
#include <algorithm>
#include <map>
#include <vector>

namespace RAT {

NoiseProc::NoiseProc() : Processor("noise") {}

void NoiseProc::BeginOfRun(DS::Run *run) {
  DBLinkPtr lnoise = DB::Get()->GetLink("NOISEPROC");

  fNoiseFlag = lnoise->GetI("noise_flag");
  fDefaultNoiseRate = lnoise->GetD("default_noise_rate");
  fLookback = lnoise->GetD("noise_lookback");
  fLookforward = lnoise->GetD("noise_lookforward");
  fMaxTime = lnoise->GetD("noise_maxtime");
  fNearHits = lnoise->GetI("noise_nearhits");

  DS::PMTInfo *pmtinfo = run->GetPMTInfo();
  UpdatePMTModels(pmtinfo);
}

Processor::Result NoiseProc::DSEvent(DS::Root *ds) {
  // Run Information
  DS::Run *run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo *pmtinfo = run->GetPMTInfo();

  // Write over MC
  DS::MC *mc = ds->GetMC();
  // Loop through current hits to get a full window and hit pmts
  double firsthittime = std::numeric_limits<double>::max();
  double lasthittime = std::numeric_limits<double>::min();

  // <pmtid, mcpmtlocation>
  std::map<int, int> mcpmtObjects;

  // <pmt hit times>
  std::vector<double> realHitTimes;

  for (int imcpmt = 0; imcpmt < mc->GetMCPMTCount(); imcpmt++) {
    DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
    mcpmtObjects[mcpmt->GetID()] = imcpmt;

    mcpmt->PruneNoiseMCPhotons();
    for (int pidx = 0; pidx < mcpmt->GetMCPhotonCount(); pidx++) {
      DS::MCPhoton *photon = mcpmt->GetMCPhoton(pidx);
      if (!photon->IsDarkHit()) {
        double hittime = photon->GetHitTime();
        if (hittime < firsthittime) {
          firsthittime = hittime;
        }
        if (hittime > lasthittime) {
          lasthittime = hittime;
        }
        realHitTimes.push_back(hittime);
      }
    }
  }

  // Cap how far forward to look in case of weird geant-4 lifetimes
  if (lasthittime > fMaxTime) {
    lasthittime = fMaxTime;
  }
  // And really just in case
  if (firsthittime < -fMaxTime) {
    firsthittime = -fMaxTime;
  }
  // When there are no hits present in an event:
  if (firsthittime > lasthittime) {
    firsthittime = 0;
    lasthittime = 0;
  }
  double noiseBegin = firsthittime - fLookback;
  double noiseEnd = lasthittime + fLookforward;

  // Look around real hits or everywhere?
  int totalNoiseHits = 0;
  if (fNearHits) {
    std::map<double, double> windowMap = FindWindows(realHitTimes, std::max(abs(fLookback), abs(fLookforward)));
    if (!windowMap.empty()) {
      for (auto m : windowMap) {
        totalNoiseHits +=
            GenerateNoiseInWindow(mc, m.first - fLookback, m.second + fLookforward, pmtinfo, mcpmtObjects);
      }
    }
  } else {
    totalNoiseHits += GenerateNoiseInWindow(mc, noiseBegin, noiseEnd, pmtinfo, mcpmtObjects);
  }
  mc->SetNumDark(totalNoiseHits);

  return Processor::OK;
}

int NoiseProc::GenerateNoiseInWindow(DS::MC *mc, double noiseBegin, double noiseEnd, DS::PMTInfo *pmtinfo,
                                     std::map<int, int> mcpmtObjects) {
  // If pmt-by-pmt noise rates are used then we have little choice but to loop
  // through each pmt -- this can be very slow for large numbers of pmts.  If
  // the pmts are indistinguishable then we can take a shortcut and generate
  // the total count first and speed things along.
  double noiseWindowWidth = noiseEnd - noiseBegin;
  int pmtCount = pmtinfo->GetPMTCount();
  int noiseHits = 0;

  if (fNoiseFlag == 0) {
    double darkCount = fDefaultNoiseRate * noiseWindowWidth * pmtCount * 1e-9;
    noiseHits = static_cast<int>(floor(CLHEP::RandPoisson::shoot(darkCount)));
    for (int ihit = 0; ihit < noiseHits; ihit++) {
      int pmtid = static_cast<int>(G4UniformRand() * pmtCount);
      if (!mcpmtObjects.count(pmtid)) {
        DS::MCPMT *mcpmt = mc->AddNewMCPMT();
        mcpmtObjects[pmtid] = mc->GetMCPMTCount() - 1;
        mcpmt->SetID(pmtid);
        mcpmt->SetType(pmtinfo->GetType(pmtid));
      }
      DS::MCPMT *mcpmt = mc->GetMCPMT(mcpmtObjects[pmtid]);
      double hittime = noiseBegin + G4UniformRand() * noiseWindowWidth;
      AddNoiseHit(mcpmt, pmtinfo, hittime);
    }
  } else {
    for (int pmtid = 0; pmtid < pmtCount; pmtid++) {
      double noiseRate = 0;
      switch (fNoiseFlag) {
        case 1: {
          const std::string modelName = pmtinfo->GetModelNameByID(pmtid);
          noiseRate = fModelNoiseMap[modelName];
          break;
        }
        case 2: {
          noiseRate = pmtinfo->GetNoiseRate(pmtid);
          break;
        }
        default: {
          noiseRate = fDefaultNoiseRate;
        }
      }
      if (noiseRate >= 0) {
        double darkCount = noiseRate * noiseWindowWidth * 1e-9;
        int idnoiseHits = static_cast<int>(floor(CLHEP::RandPoisson::shoot(darkCount)));
        for (int ihit = 0; ihit < idnoiseHits; ihit++) {
          if (!mcpmtObjects.count(pmtid)) {
            DS::MCPMT *mcpmt = mc->AddNewMCPMT();
            mcpmtObjects[pmtid] = mc->GetMCPMTCount() - 1;
            mcpmt->SetID(pmtid);
            mcpmt->SetType(pmtinfo->GetType(pmtid));
          }
          DS::MCPMT *mcpmt = mc->GetMCPMT(mcpmtObjects[pmtid]);
          double hittime = noiseBegin + G4UniformRand() * noiseWindowWidth;
          AddNoiseHit(mcpmt, pmtinfo, hittime);
        }
        noiseHits += idnoiseHits;
      } else {
        throw ParamInvalid("rate", "Noise rate must be positive");
      }
    }
  }

  return noiseHits;
}

void NoiseProc::AddNoiseHit(DS::MCPMT *mcpmt, DS::PMTInfo *pmtinfo, double hittime) {
  DS::MCPhoton *photon = mcpmt->AddNewMCPhoton();
  photon->SetDarkHit(true);
  photon->SetAfterPulse(false);
  photon->SetLambda(0.0);
  photon->SetPosition(TVector3(0, 0, 0));
  photon->SetMomentum(TVector3(0, 0, 0));
  photon->SetPolarization(TVector3(0, 0, 0));
  photon->SetTrackID(-1);
  photon->SetHitTime(hittime);
  // Modify these to check the pmt time and charge model

  photon->SetFrontEndTime(fPMTTime[pmtinfo->GetModel(mcpmt->GetID())]->PickTime(hittime));
  photon->SetCharge(fPMTCharge[pmtinfo->GetModel(mcpmt->GetID())]->PickCharge());
  mcpmt->SortMCPhotons();

  return;
}

void NoiseProc::UpdatePMTModels(DS::PMTInfo *pmtinfo) {
  const size_t numModels = pmtinfo->GetModelCount();
  fPMTTime.resize(numModels);
  fPMTCharge.resize(numModels);
  for (size_t i = 0; i < numModels; i++) {
    const std::string modelName = pmtinfo->GetModelName(i);

    try {
      fPMTTime[i] = new RAT::PDFPMTTime(modelName);
      info << "NoiseProc: Loaded PDFPMTTime for " << modelName << newline;
    } catch (DBNotFoundError &e) {
      fPMTTime[i] = new RAT::PDFPMTTime();
      info << "NoiseProc: Loaded PDFPMTTime DEFAULT for " << modelName << newline;
    }

    try {
      fPMTCharge[i] = new RAT::PDFPMTCharge(modelName);
      info << "NoiseProc: Loaded PDFPMTCharge for " << modelName << newline;
    } catch (DBNotFoundError &e) {
      fPMTCharge[i] = new RAT::PDFPMTCharge();
      info << "NoiseProc: Loaded PDFPMTCharge DEFAULT for " << modelName << newline;
    }

    try {
      DBLinkPtr lpmt = DB::Get()->GetLink("PMT", modelName);
      fModelNoiseMap[modelName] = lpmt->GetD("noise_rate");
    } catch (DBNotFoundError &e) {
      fModelNoiseMap[modelName] = fDefaultNoiseRate;
      info << "NoiseProc: By model noise rate not found for " << modelName
           << ". Using default noise rate for this model if per model rates used." << newline;
    }
  }
  return;
}

std::map<double, double> NoiseProc::FindWindows(std::vector<double> &times, double window) {
  // Sort the hit times
  std::sort(times.begin(), times.end());
  // Find the time differences along the times array
  std::map<double, double> startStop;
  // If there are too few hits, generate noise near trigger??
  if (times.size() < 2) {
    if (times.size() == 0) {
      return startStop;
    } else {
      startStop[times[0]] = times[0];
    }
    return startStop;
  }
  std::vector<double>::iterator back = times.begin();
  double start = *back;
  double stop = 0;
  for (std::vector<double>::iterator forward = next(times.begin()); forward < times.end(); ++forward) {
    if ((*forward - *back) > window) {
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

void NoiseProc::SetD(std::string param, double value) {
  if (param == "rate") {
    if (value >= 0) {
      fDefaultNoiseRate = value;
    } else {
      throw ParamInvalid(param, "Noise rate must be non-negative");
    }
  } else if (param == "lookback") {
    fLookback = abs(value);
  } else if (param == "lookforward") {
    fLookforward = abs(value);
  } else if (param == "maxtime") {
    fMaxTime = abs(value);
  } else {
    throw ParamUnknown(param);
  }
}

void NoiseProc::SetI(std::string param, int value) {
  if (param == "nearhits") {
    fNearHits = value;
  } else if (param == "flag") {
    fNoiseFlag = value;
  } else {
    throw ParamUnknown(param);
  }
}

}  // namespace RAT
