#include <CLHEP/Random/RandGauss.h>

#include <RAT/Log.hh>
#include <RAT/PMTPulse.hh>
#include <RAT/PMTWaveform.hh>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace RAT {

PMTWaveform::PMTWaveform() {

  // PMT pulse model specification
  lpulse = DB::Get()->GetLink("PMTPULSE", "lognormal");
  fPMTPulseOffset = lpulse->GetD("pulse_offset");
  fPMTPulseWidth = lpulse->GetD("pulse_width");
  fPMTPulseMean = lpulse->GetD("pulse_mean");
  fPMTPulseMin = lpulse->GetD("pulse_min");
  fPMTPulseTimeOffset = lpulse->GetD("pulse_time_offset");
  fTerminationOhms = lpulse->GetD("termination_ohms");
}

PMTWaveform::~PMTWaveform() {}

double PMTWaveform::GetHeight(double currenttime) {
  float height = 0.;
  unsigned int i = 0;
  while (i < fPulse.size() && fPulse[i]->GetPulseStartTime() <= currenttime) {
    height += fPulse[i]->GetPulseHeight(currenttime);
    i++;
  }
  return height;
}

PMTWaveform PMTWaveform::GenerateWaveforms(DS::MCPMT *mcpmt, double triggerTime) {
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
