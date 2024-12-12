/*
   After-pulsing adapted from snoplus rat. Settings for after-pulsing are controlled using AFTERPULSEProc.ratdb.
   Parameters are as follows:

   "daq": describes which index of DAQ.ratdb to pull the trigger_window value from.
   "afterpulse_flag": determines where to pull the timing distribution and after-pulse fraction from
   "0": use default timing distribution and after-pulse fraction
   "1": use per model timing dist and ap fraction
   "2": use per model timing dis and per individual ap fraction
   "afterpulse_fraction": corresponds to the fraction of events for which an after-pulse will occur
   "afterpulse_time": the time values use in the creation of the after pulse
   "afterpulse_prob": the CDF corresponding to the values in apTime used to determine the placement of an ap
   */

#include <TTimeStamp.h>

#include <RAT/AfterPulseProc.hh>
#include <RAT/DB.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <RAT/PDFPMTCharge.hh>
#include <RAT/PDFPMTTime.hh>
#include <Randomize.hh>
#include <map>
#include <random>
#include <vector>

namespace RAT {

AfterPulseProc::AfterPulseProc() : Processor("afterpulse") {}

void AfterPulseProc::BeginOfRun(DS::Run* run) {
  DBLinkPtr lafterpulse = DB::Get()->GetLink("AFTERPULSEPROC");
  fDefaultAPFraction = lafterpulse->GetD("afterpulse_fraction");
  fAPFlag = lafterpulse->GetI("afterpulse_flag");
  fDefaultAPTime = lafterpulse->GetDArray("afterpulse_time");
  fDefaultAPProb = lafterpulse->GetDArray("afterpulse_prob");
  fDAQ = lafterpulse->GetS("daq");

  fTriggerWindow = DB::Get()->GetLink("DAQ", fDAQ)->GetD("trigger_window");

  DS::PMTInfo* pmtinfo = run->GetPMTInfo();
  UpdatePMTModels(pmtinfo);
}

void AfterPulseProc::UpdatePMTModels(DS::PMTInfo* pmtinfo) {
  const size_t numModels = pmtinfo->GetModelCount();
  fPMTTime.resize(numModels);
  fPMTCharge.resize(numModels);
  for (size_t i = 0; i < numModels; i++) {
    const std::string modelName = pmtinfo->GetModelName(i);
    try {
      fPMTTime[i] = new RAT::PDFPMTTime(modelName);
      info << "AfterPulseProc: Loaded PDFPMTTime for " << modelName << newline;
    } catch (DBNotFoundError& e) {
      fPMTTime[i] = new RAT::PDFPMTTime();
      info << "AfterPulseProc: Loaded PDFPMTTime DEFAULT for " << modelName << newline;
    }
    try {
      fPMTCharge[i] = new RAT::PDFPMTCharge(modelName);
      info << "AfterPulseProc: Loaded PDFPMTCharge for " << modelName << newline;
    } catch (DBNotFoundError& e) {
      fPMTCharge[i] = new RAT::PDFPMTCharge();
      info << "AfterPulseProc: Loaded PDFPMTCharge DEFAULT for " << modelName << newline;
    }
    try {
      DBLinkPtr lmodel = DB::Get()->GetLink("AFTERPULSEPROC", modelName);
      fModelAPFractionMap[modelName] = lmodel->GetD("afterpulse_fraction");
      fModelAPProbMap[modelName] = lmodel->GetDArray("afterpulse_prob");
      fModelAPTimeMap[modelName] = lmodel->GetDArray("afterpulse_time");
    } catch (DBNotFoundError& e) {
      fModelAPFractionMap[modelName] = fDefaultAPFraction;
      fModelAPTimeMap[modelName] = fDefaultAPTime;
      fModelAPProbMap[modelName] = fDefaultAPProb;
      info << "AfterPulseProc: By model afterpulse parameters not found for " << modelName
           << ". Using default afterpulse parameters for this model if per model parameters used." << newline;
    }
  }
  return;
}

Processor::Result AfterPulseProc::DSEvent(DS::Root* ds) {
  // After pulse moved to a processor from GSim, this is a special
  // case of a processor modifying the MC branch
  // Write over MC
  DS::MC* mc = ds->GetMC();
  DS::Run* run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo* pmtinfo = run->GetPMTInfo();

  TTimeStamp timestamp = mc->GetUTC();
  double timesec = timestamp.GetSec();
  double timensec = timestamp.GetNanoSec();  // might need to add half a ns
  uint64_t eventTime = static_cast<uint64_t>(timensec + timesec * 1e9);

  std::map<int, int> mcpmtObjects;

  GenerateAfterPulses(mc, pmtinfo, eventTime);
  DetectAfterPulses(mc, pmtinfo, eventTime);

  return Processor::OK;
}

void AfterPulseProc::GenerateAfterPulses(DS::MC* mc, DS::PMTInfo* pmtinfo, const uint64_t eventTime) {
  // Afterpulses should only be generated from PMTs that have PEs on them, so use MCPMTs instead of looping over all
  // PMTs
  for (int ipmt = 0; ipmt < mc->GetMCPMTCount(); ipmt++) {
    DS::MCPMT* mcpmt = mc->GetMCPMT(ipmt);
    mcpmt->PruneAfterPulseMCPhotons();

    int pmtid = mcpmt->GetID();
    const std::string modelName = pmtinfo->GetModelNameByID(pmtid);

    double apFraction;
    std::vector<double> apTime;
    std::vector<double> apProb;

    // switch statement for pulling values from, will use the default values in the case of no flag parameter
    switch (fAPFlag) {
      case 1: {  // By model afterpulse parameters
        apFraction = fModelAPFractionMap[modelName];
        apTime = fModelAPTimeMap[modelName];
        apProb = fModelAPProbMap[modelName];
        break;
      }
      case 2: {  // By PMT afterpulse parameters
        apFraction = pmtinfo->GetAfterPulseFraction(pmtid);
        apTime = fModelAPTimeMap[modelName];
        apProb = fModelAPProbMap[modelName];
        break;
      }
      default: {  // Overall afterpulse parameters
        apFraction = fDefaultAPFraction;
        apTime = fDefaultAPTime;
        apProb = fDefaultAPProb;
      }
    }

    for (int iPE = 0; iPE < mcpmt->GetMCPhotonCount(); iPE++) {
      DS::MCPhoton* mcphoton = mcpmt->GetMCPhoton(iPE);
      if (!mcphoton->IsDarkHit()) {
        uint64_t frontEndTime = mcphoton->GetFrontEndTime();
        double rand = G4UniformRand();
        if (rand <= apFraction) {
          uint64_t afterPulseTime = CalculateAfterPulseTime(apFraction, apTime, apProb);
          uint64_t globalAPTime = eventTime + frontEndTime + afterPulseTime;
          fAfterPulseTime[pmtid].push_back(globalAPTime);
        }
      }
    }
  }
}

double AfterPulseProc::CalculateAfterPulseTime(double apFraction, std::vector<double> apTime,
                                               std::vector<double> apProb) {
  const double randtime = G4UniformRand();
  size_t up = 1;
  for (size_t i = 1; i < apTime.size(); i++) {
    if (randtime < apProb[i]) {
      up = i;
      i = apTime.size();
    }
  }
  double newtime = (randtime - apProb[(up - 1)]) * (apTime[up] - apTime[(up - 1)]) / (apProb[up] - apProb[(up - 1)]) +
                   apTime[(up - 1)];
  return newtime;
}

void AfterPulseProc::SetI(std::string param, int value) {
  if (param == "flag") {
    fAPFlag = value;
  } else {
    throw ParamUnknown(param);
  }
}

void AfterPulseProc::SetD(std::string param, double value) {
  if (param == "fraction") {
    fDefaultAPFraction = value;
  } else {
    throw ParamUnknown(param);
  }
}

void AfterPulseProc::DetectAfterPulses(DS::MC* mc, DS::PMTInfo* pmtinfo, const uint64_t eventTime) {
  // In some non-standard MC configuration, could there be an afterpulse on a PMT that
  // doesn't already have an MCPMT in a MC event? Possibly but let's ignore that for now
  for (int iPMT = 0; iPMT < mc->GetMCPMTCount(); iPMT++) {
    DS::MCPMT* mcpmt = mc->GetMCPMT(iPMT);
    int pmtid = mcpmt->GetID();
    // This is the true number of MCPhotons, which won't change after
    int nPE = mcpmt->GetMCPhotonCount();
    for (int iPE = 0; iPE < nPE; iPE++) {  // Loop over PEs
      DS::MCPhoton* mcphoton = mcpmt->GetMCPhoton(iPE);
      uint64_t frontEndTime = mcphoton->GetFrontEndTime();
      uint64_t lowWindow = eventTime + frontEndTime - static_cast<uint64_t>(2 * fTriggerWindow);
      uint64_t highWindow = eventTime + frontEndTime + static_cast<uint64_t>(2 * fTriggerWindow);

      std::vector<uint64_t> afterPulses = fAfterPulseTime[pmtid];
      std::vector<uint64_t>::iterator it = afterPulses.begin();
      while (it != afterPulses.end()) {  // Loops over APs
        uint64_t afterPulseTime = *it;   // time of after-pulse

        // If the after-pulse falls near an event window, create a PE,
        // flagged as an after-pulse. Record the time, trigger processors
        // determine whether this after-pulse actually falls into an event.
        if (afterPulseTime > lowWindow && afterPulseTime < highWindow) {
          int64_t time = afterPulseTime - eventTime;
          DS::MCPhoton* photoelectron = mcpmt->AddNewMCPhoton();

          photoelectron->SetHitTime(time);
          photoelectron->SetAfterPulse(true);
          photoelectron->SetDarkHit(false);
          // Previously the charge was just set to 1, but should sample from appropriate dist?
          photoelectron->SetCharge(fPMTCharge[pmtinfo->GetModel(mcpmt->GetID())]->PickCharge());
          // Previously a bespoke routine to sample from the time distribution, now just use the standard method a la
          // NoiseProc Is this double counting the smearing?
          photoelectron->SetFrontEndTime(fPMTTime[pmtinfo->GetModel(mcpmt->GetID())]->PickTime(time));

          // Don't double count, erase the PE from after-pulsing vector
          it = afterPulses.erase(it);
        }
        // If an event comes along well past the time of the after-pulse
        // clear the memory of the after-pulse and corresponding front-end-time.
        // NEW:
        // Old condition doesn't seem to make sense to me but I'll maintain it here
        // and put in a more sensible condition
        // else if (eventTime + frontEndTime > afterPulseTime + highWindow) {
        // Check whether afterpulse is below the low window only for the last PE
        // Assumes time-ordering so that if it's less than the low window for the last PE, it is for all
        else if (iPE == nPE - 1 && lowWindow > afterPulseTime) {
          it = afterPulses.erase(it);
        } else {
          ++it;
        }
      }  // end loop over APs
    }    // end loop over PEs
    mcpmt->SortMCPhotons();
  }  // end loop over PMTs
}
}  // namespace RAT
