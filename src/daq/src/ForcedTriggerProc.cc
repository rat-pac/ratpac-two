/*
Forced trigger causes a trigger regardless of the total number of PMTs hit
 */
#include <RAT/DS/MCPMT.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/ForcedTriggerProc.hh>

namespace RAT {

ForcedTriggerProc::ForcedTriggerProc() : Processor("forcedtrigger") {}

void ForcedTriggerProc::BeginOfRun(DS::Run *run) {
  // Trigger Specifications
  ldaq = DB::Get()->GetLink("DAQ", "ForcedTrigger");
  fEventCounter = 0;
  fDigitizerType = ldaq->GetS("digitizer_name");
  fDigitize = ldaq->GetZ("digitize");

  fDigitizer = new Digitizer(fDigitizerType);

  if (fDigitize) {
    DS::PMTInfo *pmtinfo = run->GetPMTInfo();
    const size_t numModels = pmtinfo->GetModelCount();
    for (size_t i = 0; i < numModels; i++) {
      const std::string &modelName = pmtinfo->GetModelName(i);
      fDigitizer->AddWaveformGenerator(modelName);
    }
  }
}

Processor::Result ForcedTriggerProc::DSEvent(DS::Root *ds) {
  DS::MC *mc = ds->GetMC();
  DS::Run *run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo *pmtinfo = run->GetPMTInfo();
  // Prune the previous EV branchs if one exists
  if (ds->ExistEV()) ds->PruneEV();

  DS::EV *ev = ds->AddNewEV();
  ev->SetID(fEventCounter++);
  ev->SetCalibratedTriggerTime(0.0);
  ev->SetUTC(mc->GetUTC());
  double totalEVCharge = 0;
  // Loop over the mcpmts and fill the pmt branch assuming we've triggered
  for (int imcpmt = 0; imcpmt < mc->GetMCPMTCount(); imcpmt++) {
    DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
    int pmtID = mcpmt->GetID();
    double integratedCharge = 0;
    double time = 0;
    for (int pidx = 0; pidx < mcpmt->GetMCPhotonCount(); pidx++) {
      DS::MCPhoton *photon = mcpmt->GetMCPhoton(pidx);
      // Use the first PE time as the pmt time
      if (pidx == 0) {
        time = photon->GetFrontEndTime();
      }
      integratedCharge += photon->GetCharge();
    }
    DS::PMT *pmt = ev->GetOrCreatePMT(pmtID);
    pmt->SetTime(time);
    pmt->SetCharge(integratedCharge);
    totalEVCharge += integratedCharge;
    if (fDigitize) {
      fDigitizer->DigitizePMT(mcpmt, pmtID, 0.0, pmtinfo);
    }
  }
  if (fDigitize) {
    fDigitizer->WriteToEvent(ev);
  }
  ev->SetTotalCharge(totalEVCharge);

  return Processor::OK;
}

}  // namespace RAT
