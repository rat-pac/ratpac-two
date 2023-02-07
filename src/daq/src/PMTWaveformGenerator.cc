#include <RAT/PMTWaveformGenerator.hh>
#include <iostream>

namespace RAT {

PMTWaveformGenerator::PMTWaveformGenerator() {
  // PMT pulse model specification
  lpulse = DB::Get()->GetLink("PMTPULSE", "lognormal");
  fPMTPulseOffset = lpulse->GetD("pulse_offset");
  fPMTPulseWidth = lpulse->GetD("pulse_width");
  fPMTPulseMean = lpulse->GetD("pulse_mean");
  fPMTPulseMin = lpulse->GetD("pulse_min");
  fPMTPulseTimeOffset = lpulse->GetD("pulse_time_offset");
  fTerminationOhms = lpulse->GetD("termination_ohms");
}

PMTWaveformGenerator::~PMTWaveformGenerator() {}

PMTWaveform PMTWaveformGenerator::GenerateWaveforms(DS::MCPMT *mcpmt, double triggerTime) {
  PMTWaveform pmtwf;

  // Loop over PEs and create a pulse for each one
  for (int iph = 0; iph < mcpmt->GetMCPhotonCount(); iph++) {
    DS::MCPhoton *mcpe = mcpmt->GetMCPhoton(iph);

    PMTPulse *pmtpulse = new PMTPulse;
    pmtpulse->SetPulseCharge(mcpe->GetCharge() * fTerminationOhms);
    pmtpulse->SetPulseMin(fPMTPulseMin);
    pmtpulse->SetPulseOffset(fPMTPulseOffset);
    pmtpulse->SetPulseTimeOffset(fPMTPulseTimeOffset);
    pmtpulse->SetPulseWidth(fPMTPulseWidth);
    pmtpulse->SetPulseMean(fPMTPulseMean);
    pmtpulse->SetPulseStartTime(mcpe->GetFrontEndTime() - triggerTime);
    pmtwf.fPulse.push_back(pmtpulse);
  }

  return pmtwf;
}


}  // namespace RAT
