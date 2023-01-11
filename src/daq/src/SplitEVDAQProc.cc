/* SplitEVDAQ is an extension of the SimpleDAQ that converts hits into multiple
 * triggers, and places those in new events (not subevents) to better simulate
 * data and produce combined datasets.
 */
#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/PMTPulse.hh>
#include <RAT/SplitEVDAQProc.hh>
#include <algorithm>
#include <vector>

namespace RAT {

SplitEVDAQProc::SplitEVDAQProc() : Processor("splitevdaq") {
  // Trigger Specifications
  ldaq = DB::Get()->GetLink("SplitEVDAQ");
  fEventCounter = 0;
  fPulseWidth = ldaq->GetD("pulse_width");
  fTriggerThreshold = ldaq->GetD("trigger_threshold");
  fTriggerWindow = ldaq->GetD("trigger_window");
  fPmtLockout = ldaq->GetD("pmt_lockout");
  fTriggerLockout = ldaq->GetD("trigger_lockout");
  fTriggerResolution = ldaq->GetD("trigger_resolution");
  fLookback = ldaq->GetD("lookback");
  fMaxHitTime = ldaq->GetD("max_hit_time");
  fPmtType = ldaq->GetI("pmt_type");
  fTriggerOnNoise = ldaq->GetI("trigger_on_noise");
  fTerminationOhms = ldaq->GetD("termination_ohms");
  fDigitizerType = ldaq->GetS("digitizer_name");
  fDigitize = ldaq->GetZ("digitize");

  fDigitizer = new Digitizer(fDigitizerType);

  fWaveformAnalysis = new WaveformAnalysis();

  // PMT pulse model specification (for digitization)
  lpulse = DB::Get()->GetLink("PMTPULSE", "lognormal");
  fPMTPulseOffset = lpulse->GetD("pulse_offset");
  fPMTPulseWidth = lpulse->GetD("pulse_width");
  fPMTPulseMean = lpulse->GetD("pulse_mean");
  fPMTPulseMin = lpulse->GetD("pulse_min");
  fPMTPulseTimeOffset = lpulse->GetD("pulse_time_offset");
}

PMTWaveform SplitEVDAQProc::GenerateWaveforms(DS::MCPMT *mcpmt, double tt) {
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
    pmtpulse->SetPulseStartTime(mcpe->GetFrontEndTime()-tt);
    pmtwf.fPulse.push_back(pmtpulse);
  }

  return pmtwf;
}

Processor::Result SplitEVDAQProc::DSEvent(DS::Root *ds) {
  // This DAQ will convert hits on PMTs through time into trigger pulses
  // which can fire a global event trigger. Each trigger will correspond
  // to a single sub-event in the datastructure.
  // Not included yet
  // - Noise on the trigger pulse height, rise-time, etc
  // - Disciminator on charge (all hits assumed to trigger)

  DS::MC *mc = ds->GetMC();
  // Prune the previous EV branchs if one exists
  if (ds->ExistEV()) ds->PruneEV();

  // First loop through the PMTs and create a summed trigger
  std::vector<double> trigPulses;
  for (int imcpmt = 0; imcpmt < mc->GetMCPMTCount(); imcpmt++) {
    DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
    double lastTrigger = -100000.0;
    for (int pidx = 0; pidx < mcpmt->GetMCPhotonCount(); pidx++) {
      DS::MCPhoton *photon = mcpmt->GetMCPhoton(pidx);
      // Do we want to trigger on noise hits?
      if (!fTriggerOnNoise && photon->IsDarkHit()) continue;
      double time = photon->GetFrontEndTime();
      if (time > fMaxHitTime) continue;
      if (time > (lastTrigger + fPmtLockout)) {
        trigPulses.push_back(time);
        lastTrigger = time;
      }
    }
  }
  if (trigPulses.size() < 1) return Processor::OK;  // We're done, no triggers

  double start = *std::min_element(trigPulses.begin(), trigPulses.end());
  start = floor(start / fTriggerResolution) * fTriggerResolution;
  double end = *std::max_element(trigPulses.begin(), trigPulses.end());
  end = (floor(end / fTriggerResolution) + 1) * fTriggerResolution;
  std::sort(trigPulses.begin(), trigPulses.end());

  // Turns hits into a histogram of trigger pulse leading edges
  //        _
  //   _   | |    _
  // _| |__| |___| |___
  int nbins = floor((end - start) / fTriggerResolution) + 1;
  double bw = fTriggerResolution;
  std::vector<double> triggerTrain(nbins);
  for (auto v : trigPulses) {
    int select = int((v - start) / bw);
    triggerTrain[select] += 1.0;
  }

  // Spread each bin out to the trigger pulse width to pass to a discriminator
  //             |
  //.............|_____ .... trigger threshold .....
  //        _____|     |____
  //       |     |          |____
  //   ____|     |               |____
  // _|          | global trigger!    |___
  std::vector<double> triggerHistogram(nbins);
  for (int i = 0; i < nbins; i++) {
    double x = triggerTrain[i];
    if (x > 0) {
      for (int j = i; j < i + fPulseWidth; j++) {
        if (j >= nbins) break;
        triggerHistogram[j] += x;
      }
    }
  }

  // Trigger the detector based on fTriggerThreshold
  double lastTrigger = -100000.0;
  std::vector<double> triggerTimes;
  for (int i = 0; i < nbins; i++) {
    double v = triggerHistogram[i];
    if (v >= fTriggerThreshold)  // check for trigger
    {
      if ((i * bw) + start > (lastTrigger + fTriggerWindow + fTriggerLockout)) {
        lastTrigger = (i * bw) + start;
        triggerTimes.push_back(lastTrigger);
      }
    }
  }

  // Place the correct hits, charges, etc into the right trigger windows
  lastTrigger = 0;
  for (auto tt : triggerTimes) {
    DS::EV *ev = ds->AddNewEV();
    ev->SetID(fEventCounter++);
    ev->SetCalibratedTriggerTime(tt);
    ev->SetDeltaT(tt - lastTrigger);
    lastTrigger = tt;
    double totalEVCharge = 0;  // What does total charge get used for?
    for (int imcpmt = 0; imcpmt < mc->GetMCPMTCount(); imcpmt++) {
      DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
      int pmtID = mcpmt->GetID();
      // Check if the mcpmt has a time within one pulsewidth of the trigger window
      bool pmtInEvent = false;
      double integratedCharge = 0;
      std::vector<double> hitTimes;
      if (mcpmt->GetMCPhotonCount() > 0) {
        for (int pidx = 0; pidx < mcpmt->GetMCPhotonCount(); pidx++) {
          DS::MCPhoton *photon = mcpmt->GetMCPhoton(pidx);
          double time = photon->GetFrontEndTime();
          if ((time > (tt - fLookback)) && (time < (tt + fTriggerWindow))) {
            pmtInEvent = true;
            hitTimes.push_back(time);
            integratedCharge += photon->GetCharge();
          }
        }
      }
      std::sort(hitTimes.begin(), hitTimes.end());
      if (pmtInEvent) {
        DS::PMT *pmt = ev->AddNewPMT();
        pmt->SetID(pmtID);
        double front_end_hit_time = *std::min_element(hitTimes.begin(), hitTimes.end());
        // PMT Hit time relative to the trigger
        pmt->SetTime(front_end_hit_time - tt);
        pmt->SetCharge(integratedCharge);
        totalEVCharge += integratedCharge;
        if (fDigitize) {
          PMTWaveform pmtwfm = GenerateWaveforms(mcpmt,tt);
          fDigitizer->AddChannel(pmtID, pmtwfm);
          double dy = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
          double digit_time = fWaveformAnalysis->CalculateTime(
              fDigitizer->fDigitWaveForm[pmtID], fDigitizer->fPedWindowLow, fDigitizer->fPedWindowHigh, dy,
              fDigitizer->fSamplingRate, fDigitizer->fConstFrac, fDigitizer->fLookback);

          double time_step = 1.0 / fDigitizer->fSamplingRate;
          int low_int_window = int((digit_time - fDigitizer->fIntWindowLow) / time_step);
          int high_int_window = int((digit_time + fDigitizer->fIntWindowHigh) / time_step);

          double digit_charge = fWaveformAnalysis->Integrate(
              fDigitizer->fDigitWaveForm[pmtID], fDigitizer->fPedWindowLow, fDigitizer->fPedWindowHigh, dy,
              fDigitizer->fSamplingRate, low_int_window, high_int_window, fTerminationOhms);
          pmt->SetDigitizedTime(digit_time);
          pmt->SetDigitizedCharge(digit_charge);
        }
      }
    }  // Done looping over PMTs

    if (fDigitize) {
      std::map<UShort_t, std::vector<UShort_t>> waveforms = fDigitizer->fDigitWaveForm;
      for (std::map<UShort_t, std::vector<UShort_t>>::const_iterator it = waveforms.begin(); it != waveforms.end();
           it++) {
        digit.SetWaveform(UShort_t(it->first), waveforms[UShort_t(it->first)]);
      }

      digit.SetDigitName(fDigitizer->fDigitName);
      digit.SetNSamples(UShort_t(fDigitizer->fNSamples));
      digit.SetNBits(UShort_t(fDigitizer->fNBits));
      digit.SetDynamicRange((fDigitizer->fVhigh - fDigitizer->fVlow));
      digit.SetSamplingRate(fDigitizer->fSamplingRate);

      ev->SetDigitizer(digit);
    }

    ev->SetTotalCharge(totalEVCharge);
  }

  return Processor::OK;
}

void SplitEVDAQProc::SetD(std::string param, double value) {
  if (param == "pulse_width")
    fPulseWidth = value;
  else if (param == "trigger_threshold")
    fTriggerThreshold = value;
  else if (param == "trigger_window")
    fTriggerWindow = value;
  else if (param == "pmt_lockout")
    fPmtLockout = value;
  else if (param == "trigger_lockout")
    fTriggerLockout = value;
  else if (param == "trigger_resolution")
    fTriggerResolution = value;
  else if (param == "lookback")
    fLookback = value;
  else if (param == "max_hit_time")
    fMaxHitTime = value;
  else
    throw ParamUnknown(param);
}

void SplitEVDAQProc::SetI(std::string param, int value) {
  if (param == "pulse_type")
    fPmtType = value;
  else if (param == "trigger_on_noise")
    fTriggerOnNoise = value;
  else
    throw ParamUnknown(param);
}

}  // namespace RAT
