#include <RAT/PMTWaveformGenerator.hh>
#include <iostream>

namespace RAT {

PMTWaveformGenerator::PMTWaveformGenerator(std::string modelName) {
  // PMT pulse model specification
  fModelName = modelName;
  try {
    lpulse = DB::Get()->GetLink("PMTPULSE", modelName);
    lpulse->GetS("index");
    info << "PMTWaveformGenerator: Found pulse table for " << modelName << "." << newline;
  } catch (DBNotFoundError &e) {
    info << "PMTWaveformGenerator: Could not find pulse table for " << modelName << ". Trying default." << newline;
    try {
      lpulse = DB::Get()->GetLink("PMTPULSE", "");
      lpulse->GetS("index");
      info << "PMTWaveformGenerator: Using default pulse table for " << modelName << "." << newline;
    } catch (DBNotFoundError &e) {
      Log::Die("PMTWaveformGenerator: No default pulse table found.");
    }
  }
  try {
    fPMTPulseType = lpulse->GetS("pulse_type");
    info << "PMTWaveformGenerator: Using " << fPMTPulseType << " pulse for " << modelName << "." << newline;
  } catch (DBNotFoundError &e) {
    info << "PMTWaveformGenerator: Could not find pulse type for " << modelName << ". Using analytic as default."
         << newline;
    fPMTPulseShape = "analytic";
  }

  fPMTPulseShape == "";
  if (fPMTPulseType == "analytic") {
    try {
      fPMTPulseShape = lpulse->GetS("pulse_shape");
      info << "PMTWaveformGenerator: Using " << fPMTPulseShape << " pulse for " << modelName << "." << newline;
    } catch (DBNotFoundError &e) {
      info << "PMTWaveformGenerator: Could not find pulse shape for " << modelName << ". Using lognormal as default."
           << newline;
      fPMTPulseShape = "lognormal";
    }
  }

  fPMTPulseOffset = lpulse->GetD("pulse_offset");
  fPMTPulseMin = lpulse->GetD("pulse_min");
  fPMTPulseTimeOffset = lpulse->GetD("pulse_time_offset");
  fTerminationOhms = lpulse->GetD("termination_ohms");
  fPMTPulsePolarity = lpulse->GetZ("pulse_polarity_negative");

  if (fPMTPulseType == "analytic") {
    if (fPMTPulseShape == "lognormal") {
      fPMTPulseWidth = lpulse->GetD("pulse_width");
      fPMTPulseMean = lpulse->GetD("pulse_mean");
    }
  }
  if (fPMTPulseType == "datadriven") {
    fPMTPulseShapeTimes = lpulse->GetDArray("pulse_shape_times");
    fPMTPulseShapeValues = lpulse->GetDArray("pulse_values_times");
  }
}

PMTWaveformGenerator::~PMTWaveformGenerator() {}

PMTWaveform PMTWaveformGenerator::GenerateWaveforms(DS::MCPMT *mcpmt, double triggerTime) {
  PMTWaveform pmtwf;

  // Loop over PEs and create a pulse for each one
  for (int iph = 0; iph < mcpmt->GetMCPhotonCount(); iph++) {
    DS::MCPhoton *mcpe = mcpmt->GetMCPhoton(iph);

    pmtwf.fPulse.push_back(PMTPulse(fPMTPulseType, fPMTPulseShape));
    PMTPulse *pmtpulse = &pmtwf.fPulse.back();
    pmtpulse->SetPulseCharge(mcpe->GetCharge() * fTerminationOhms);
    pmtpulse->SetPulseMin(fPMTPulseMin);
    pmtpulse->SetPulseOffset(fPMTPulseOffset);
    pmtpulse->SetPulseTimeOffset(fPMTPulseTimeOffset);
    pmtpulse->SetPulseStartTime(mcpe->GetFrontEndTime() - triggerTime);
    pmtpulse->SetPulsePolarity(fPMTPulsePolarity);

    pmtpulse->SetPulseWidth(fPMTPulseWidth);
    pmtpulse->SetPulseMean(fPMTPulseMean);

    pmtpulse->SetPulseTimes(fPMTPulseShapeTimes);
    pmtpulse->SetPulseValues(fPMTPulseShapeValues);
  }

  return pmtwf;
}

}  // namespace RAT
