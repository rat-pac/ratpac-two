#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformPrep.hh>
#include <RAT/WaveformUtil.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/RunStore.hh"

namespace RAT {

WaveformPrep::WaveformPrep() : WaveformPrep::WaveformPrep("") {}

WaveformPrep::WaveformPrep(std::string analyzer_name) : Processor("WaveformPrep") { Configure(analyzer_name); }

void WaveformPrep::Configure(const std::string& analyzer_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", analyzer_name);
    fPedWindowLow = fDigit->GetI("pedestal_window_low");
    fPedWindowHigh = fDigit->GetI("pedestal_window_high");
    fLookback = fDigit->GetD("lookback");
    fIntWindowLow = fDigit->GetD("integration_window_low");
    fIntWindowHigh = fDigit->GetD("integration_window_high");
    fConstFrac = fDigit->GetD("constant_fraction");
    fThreshold = fDigit->GetD("voltage_threshold");
    fSlidingWindow = fDigit->GetD("sliding_window_width");
    fChargeThresh = fDigit->GetD("sliding_window_thresh");
    fApplyCableOffset = fDigit->GetI("apply_cable_offset");
    fZeroSuppress = fDigit->GetI("zero_suppress");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysis: Unable to find analysis parameters.");
  }
}

void WaveformPrep::SetS(std::string param, std::string value) {
  if (param == "analyzer_name") {
    Configure(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformPrep::SetI(std::string param, int value) {
  if (param == "pedestal_window_low") {
    fPedWindowLow = value;
  } else if (param == "pedestal_window_high") {
    fPedWindowHigh = value;
  } else if (param == "apply_cable_offset") {
    fApplyCableOffset = value;
  } else if (param == "zero_suppress") {
    fZeroSuppress = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformPrep::SetD(std::string param, double value) {
  if (param == "lookback") {
    fLookback = value;
  } else if (param == "integration_window_low") {
    fIntWindowLow = value;
  } else if (param == "integration_window_high") {
    fIntWindowHigh = value;
  } else if (param == "constant_fraction") {
    fConstFrac = value;
  } else if (param == "voltage_threshold") {
    fThreshold = value;
  } else if (param == "sliding_window_width") {
    fSlidingWindow = value;
  } else if (param == "sliding_window_thresh") {
    fChargeThresh = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformPrep::ZeroSuppress(DS::EV* ev, DS::DigitPMT* digitpmt, int pmtID) {
  if (fZeroSuppress) {
    if (digitpmt->GetNCrossings() <= 0) {
      size_t nerased = ev->EraseDigitPMT(pmtID);
      if (nerased != 1)
        warn << "WaveformPrep: Removed " << nerased << " digitPMTs with a single call to EraseDigitPMT. Impossible!"
             << newline;
    }
  }
}

void WaveformPrep::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, Digitizer* fDigitizer, double timeOffset) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  std::vector<UShort_t> digitWfm = fDigitizer->fDigitWaveForm[pmtID];
  fTermOhms = fDigitizer->fTerminationOhms;
  DoAnalysis(digitpmt, digitWfm, timeOffset);
}

void WaveformPrep::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, DS::Digit* dsdigit, double timeOffset) {
  fVoltageRes = dsdigit->GetVoltageResolution();
  fTimeStep = dsdigit->GetTimeStepNS();
  fTermOhms = dsdigit->GetTerminationOhms();
  std::vector<UShort_t> digitWfm = dsdigit->GetWaveform(pmtID);
  DoAnalysis(digitpmt, digitWfm, timeOffset);
}

void WaveformPrep::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm, double timeOffset) {
  // Calculate baseline in ADC units
  double pedestal = WaveformUtil::CalculatePedestalADC(digitWfm, fPedWindowLow, fPedWindowHigh);

  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);

  // Calculate highest peak in mV
  std::pair<int, double> peak = WaveformUtil::FindHighestPeak(voltWfm);
  int samplePeak = peak.first;
  double voltagePeak = peak.second;

  // Get the total number of threshold crossings
  std::tuple<int, double, double> crossingsInfo = WaveformUtil::GetCrossingsInfo(voltWfm, fThreshold, fTimeStep);
  int nCrossings = std::get<0>(crossingsInfo);
  double timeOverThreshold = std::get<1>(crossingsInfo);
  double voltageOverThreshold = std::get<2>(crossingsInfo);

  // Calculate the constant-fraction hit-time
  double digitTime = WaveformUtil::INVALID;
  if (nCrossings > 0) digitTime = WaveformUtil::CalculateTimeCFD(voltWfm, samplePeak, fLookback, fTimeStep, fConstFrac);

  // Integrate the waveform to calculate the charge
  double charge = WaveformUtil::IntegratePeak(voltWfm, samplePeak, fIntWindowLow, fIntWindowHigh, fTimeStep, fTermOhms);
  double totalCharge = WaveformUtil::IntegrateSliding(voltWfm, fSlidingWindow, fChargeThresh, fTimeStep, fTermOhms);

  digitpmt->SetTimeOffset(timeOffset);
  digitpmt->SetDigitizedTime(digitTime);
  digitpmt->SetDigitizedCharge(charge);
  digitpmt->SetDigitizedTotalCharge(totalCharge);
  digitpmt->SetNCrossings(nCrossings);
  digitpmt->SetTimeOverThreshold(timeOverThreshold);
  digitpmt->SetVoltageOverThreshold(voltageOverThreshold);
  digitpmt->SetPedestal(pedestal);
  digitpmt->SetPeakVoltage(voltagePeak);
}

double WaveformPrep::RunAnalysisOnTrigger(int pmtID, Digitizer* fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  std::vector<UShort_t> digitWfm = fDigitizer->fDigitWaveForm[pmtID];

  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes);

  double trigger_threshold = fThreshold;
  double trigger_lookback = fLookback;
  try {
    trigger_threshold = fDigit->GetD("trigger_voltage_threshold");
    trigger_lookback = fDigit->GetD("trigger_lookback");
  } catch (DBNotFoundError& e) {
    warn << "WaveformPrep: Trigger threshold and lookback not found in database. "
         << "Using the same parameters as PMT Waveforms." << newline;
  }
  fVoltageRes *= -1;  // Invert the voltage since the waveform goes ABOVE threshold when a trigger occurs
  trigger_threshold *= -1;
  // Calculate highest peak in mV
  std::pair<int, double> peak = WaveformUtil::FindHighestPeak(voltWfm);
  int samplePeak = peak.first;
  // HACK: Store the old lookback value, restore after a trigger time analysis is done.
  double old_lookback = fLookback;
  fLookback = trigger_lookback;
  double trigger_time = WaveformUtil::CalculateTimeCFD(voltWfm, samplePeak, fLookback, fTimeStep, WaveformUtil::INVALID,
                                                       trigger_threshold);
  fLookback = old_lookback;
  return trigger_time;
}

Processor::Result WaveformPrep::Event(DS::Root* ds, DS::EV* ev) {
  if (!ev->DigitizerExists()) {
    warn << "Running waveform analysis, but no digitzer information." << newline;
    return Processor::Result::OK;
  }
  DS::Digit* dsdigit = &ev->GetDigitizer();
  DS::Run* run = DS::RunStore::GetRun(ds->GetRunID());
  const DS::ChannelStatus* ch_status = run->GetChannelStatus();
  std::vector<int> pmt_ids = dsdigit->GetIDs();
  double total_charge = 0;
  double time_offset = 0;
  for (int pmt_id : pmt_ids) {
    // Do not analyze negative pmtid channels, since they do not correspond to real PMTs.
    if (pmt_id < 0) continue;
    if (!ch_status->GetOnlineByPMTID(pmt_id)) continue;
    DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(pmt_id);
    time_offset = fApplyCableOffset ? ch_status->GetCableOffsetByPMTID(pmt_id) : 0.0;
    RunAnalysis(digitpmt, pmt_id, dsdigit, time_offset);
    if (digitpmt->GetNCrossings() > 0) {
      total_charge += digitpmt->GetDigitizedCharge();
    }
    ZeroSuppress(ev, digitpmt, pmt_id);
    ev->SetTotalCharge(total_charge);
  }
  return Processor::Result::OK;
}

}  // namespace RAT
