#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysis.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/RunStore.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"

namespace RAT {

WaveformAnalysis::WaveformAnalysis() : WaveformAnalysis::WaveformAnalysis("") {}

WaveformAnalysis::WaveformAnalysis(std::string analyzer_name) : Processor("WaveformAnalysis") {
  warn << "This class is deprecated -- please use WaveformPrep and WaveformAnalysisLogNormal" << newline;
  Configure(analyzer_name);
}

void WaveformAnalysis::Configure(const std::string& analyzer_name) {
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
    fRunFit = fDigit->GetI("run_fitting");
    fApplyCableOffset = fDigit->GetI("apply_cable_offset");
    fZeroSuppress = fDigit->GetI("zero_suppress");
    if (fRunFit) {
      fFitWindowLow = fDigit->GetD("fit_window_low");
      fFitWindowHigh = fDigit->GetD("fit_window_high");
      fFitShape = fDigit->GetD("lognormal_shape");
      fFitScale = fDigit->GetD("lognormal_scale");
    }
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysis: Unable to find analysis parameters.");
  }
}

void WaveformAnalysis::SetS(std::string param, std::string value) {
  if (param == "analyzer_name") {
    Configure(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysis::SetI(std::string param, int value) {
  if (param == "run_fitting") {
    fRunFit = value;
  } else if (param == "pedestal_window_low") {
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

void WaveformAnalysis::SetD(std::string param, double value) {
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
  } else if (param == "fit_window_low") {
    fFitWindowLow = value;
  } else if (param == "fit_window_high") {
    fFitWindowHigh = value;
  } else if (param == "lognormal_shape") {
    fFitShape = value;
  } else if (param == "lognormal_scale") {
    fFitScale = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysis::ZeroSuppress(DS::EV* ev, DS::DigitPMT* digitpmt, int pmtID) {
  if (fZeroSuppress) {
    if (digitpmt->GetNCrossings() <= 0) {
      size_t nerased = ev->EraseDigitPMT(pmtID);
      if (nerased != 1)
        warn << "WaveformAnalysis: Removed " << nerased << " digitPMTs with a single call to EraseDigitPMT. Impossible!"
             << newline;
    }
  }
}

void WaveformAnalysis::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, Digitizer* fDigitizer, double timeOffset) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  fDigitWfm = fDigitizer->fDigitWaveForm[pmtID];
  fTermOhms = fDigitizer->fTerminationOhms;
  DoAnalysis(digitpmt, timeOffset);
}

void WaveformAnalysis::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, DS::Digit* dsdigit, double timeOffset) {
  fVoltageRes = dsdigit->GetVoltageResolution();
  fTimeStep = dsdigit->GetTimeStepNS();
  fDigitWfm = dsdigit->GetWaveform(pmtID);
  fTermOhms = dsdigit->GetTerminationOhms();
  DoAnalysis(digitpmt, timeOffset);
}

void WaveformAnalysis::DoAnalysis(DS::DigitPMT* digitpmt, double timeOffset) {
  // Calculate baseline in mV
  CalculatePedestal();

  // Calculate peak in mV
  GetPeak();

  // Calculate the constant-fraction hit-time
  CalculateTimeCFD();

  // Get the total number of threshold crossings
  GetNCrossings();

  // Integrate the waveform to calculate the charge
  Integrate();
  SlidingIntegral();

  if (fRunFit) {
    FitWaveform();
  }

  digitpmt->SetTimeOffset(timeOffset);
  digitpmt->SetDigitizedTime(fDigitTime);
  digitpmt->SetDigitizedCharge(fCharge);
  digitpmt->SetDigitizedTotalCharge(fTotalCharge);
  digitpmt->SetNCrossings(fNCrossings);
  digitpmt->SetTimeOverThreshold(fTimeOverThreshold);
  digitpmt->SetVoltageOverThreshold(fVoltageOverThreshold);
  digitpmt->SetPedestal(fPedestal);
  digitpmt->SetPeakVoltage(fVoltagePeak);
  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("Lognormal");
  fit_result->AddPE(fFittedTime - timeOffset, fFittedHeight,
                    {{"baseline", fFittedBaseline}, {"interpolated_time", fInterpolatedTime}});
  digitpmt->SetFittedBaseline(fFittedBaseline);
  digitpmt->SetFittedHeight(fFittedHeight);
  digitpmt->SetFittedTime(fFittedTime - timeOffset);
}

double WaveformAnalysis::RunAnalysisOnTrigger(int pmtID, Digitizer* fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  fDigitWfm = fDigitizer->fDigitWaveForm[pmtID];

  CalculatePedestal();
  double trigger_threshold = fThreshold;
  double trigger_lookback = fLookback;
  try {
    trigger_threshold = fDigit->GetD("trigger_voltage_threshold");
    trigger_lookback = fDigit->GetD("trigger_lookback");
  } catch (DBNotFoundError& e) {
    warn << "WaveformAnalysis: Trigger threshold and lookback not found in database. "
         << "Using the same parameters as PMT Waveforms." << newline;
  }
  fVoltageRes *= -1;  // Invert the voltage since the waveform goes ABOVE threshold when a trigger occurs
  trigger_threshold *= -1;
  GetPeak();
  // HACK: Store the old lookback value, restore after a trigger time analysis is done.
  double old_lookback = fLookback;
  fLookback = trigger_lookback;
  double trigger_time = CalculateThresholdCrossingTime(trigger_threshold);
  fLookback = old_lookback;
  return trigger_time;
}

void WaveformAnalysis::CalculatePedestal() {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  fPedestal = 0;

  if (fPedWindowLow > fDigitWfm.size()) {
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than waveform size.");
  } else if (fPedWindowLow > fPedWindowHigh) {
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than end of pedestal window.");
  }

  // Ensure end of pedestal window is less than waveform size
  fPedWindowHigh = (fPedWindowHigh > fDigitWfm.size()) ? fDigitWfm.size() : fPedWindowHigh;

  for (UShort_t i = fPedWindowLow; i < fPedWindowHigh; i++) {
    fPedestal += fDigitWfm[i];
  }
  fPedestal /= (fPedWindowHigh - fPedWindowLow);
}

void WaveformAnalysis::Interpolate(double voltage1, double voltage2) {
  /*
  Linearly interpolate between two samples
  */
  double deltav = (voltage1 - voltage2);
  double dx = (fVoltageCrossing - voltage2) / deltav;
  fInterpolatedTime = dx * fTimeStep;
}

void WaveformAnalysis::GetPeak() {
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  fVoltagePeak = 999;
  fSamplePeak = -999;
  for (size_t i = 0; i < fDigitWfm.size(); i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);

    // Downward going pulse
    if (voltage < fVoltagePeak) {
      fVoltagePeak = voltage;
      fSamplePeak = i;
    }
  }
}

void WaveformAnalysis::GetThresholdCrossing() {
  /*
  Identifies the sample at which threshold crossing occurs
   */
  fThresholdCrossing = 0;
  // Make sure we don't scan passed the beginning of the waveform
  Int_t lb = Int_t(fSamplePeak) - Int_t(fLookback / fTimeStep);
  UShort_t back_window = (lb > 0) ? lb : 0;

  // Start at the peak and scan backwards
  for (UShort_t i = fSamplePeak; i > back_window; i--) {
    double voltage = (fDigitWfm[i] - fPedestal) * fVoltageRes;

    if (voltage > fVoltageCrossing) {
      fThresholdCrossing = i;
      break;
    }

    // Reached the begining of the waveform
    // returned an invalid value
    if (i == back_window) {
      fThresholdCrossing = INVALID;
      break;
    }
  }
}

void WaveformAnalysis::GetNCrossings() {
  /*
  Calculates the total number of threshold crossings
  */
  fNCrossings = 0;
  fTimeOverThreshold = 0;
  fVoltageOverThreshold = 0;

  bool fCrossed = false;
  // Scan over the entire waveform
  for (UShort_t i = 0; i < fDigitWfm.size(); i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);

    // If we crossed below threshold
    if (voltage < fThreshold) {
      // Not already below thresh, count the crossing
      if (!fCrossed) {
        fNCrossings += 1;
      }
      // Count the time over threshold, mark that we crossed
      fTimeOverThreshold += fTimeStep;
      fVoltageOverThreshold += voltage;
      fCrossed = true;
    }
    // If we are above threshold
    if (voltage > fThreshold) {
      fCrossed = false;
    }
  }
}

double WaveformAnalysis::CalculateThresholdCrossingTime() {
  /*
  Calculate the time a threshold crossing occurs, with a linear interpolation
  */
  // Get the sample where the voltage thresh is crossed
  GetThresholdCrossing();

  if (fThresholdCrossing == INVALID || fThresholdCrossing >= fDigitWfm.size()) {
    return INVALID;
  }

  if (fThresholdCrossing >= fDigitWfm.size()) {
    Log::Die("WaveformAnalysis: Threshold crossing sample larger than waveform window.");
  }

  // Interpolate between the two samples where the CFD threshold is crossed
  double v1 = DigitToVoltage(fDigitWfm[fThresholdCrossing + 1]);
  double v2 = DigitToVoltage(fDigitWfm[fThresholdCrossing]);
  Interpolate(v1, v2);

  double interpolated_time = double(fThresholdCrossing) * fTimeStep + fInterpolatedTime;

  return interpolated_time;
}

void WaveformAnalysis::CalculateTimeCFD() {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */
  fVoltageCrossing = fConstFrac * fVoltagePeak;
  fDigitTime = CalculateThresholdCrossingTime();
}

void WaveformAnalysis::Integrate() {
  /*
  Integrate the digitized waveform around the peak to calculate charge
  */
  fCharge = 0;

  fLowIntWindow = fSamplePeak - fIntWindowLow;
  fHighIntWindow = fSamplePeak + fIntWindowHigh;

  if (fLowIntWindow >= fDigitWfm.size()) {
    fCharge = INVALID;
    return;
  }

  // Make sure not to integrate past the end of the waveform
  fHighIntWindow = (fHighIntWindow > fDigitWfm.size()) ? fDigitWfm.size() : fHighIntWindow;
  // Make sure not to integrate before the waveform starts
  fLowIntWindow = (fLowIntWindow < 0) ? 0 : fLowIntWindow;

  for (size_t i = fLowIntWindow; i < fHighIntWindow; i++) {
    double voltage = DigitToVoltage(fDigitWfm[i]);
    fCharge += (-voltage * fTimeStep) / fTermOhms;  // in pC
  }
}

void WaveformAnalysis::SlidingIntegral() {
  /*
  Integrate the digitized waveform over sliding windows to calculate a total charge
  */
  fTotalCharge = 0;
  int nsliding = int(fDigitWfm.size() / fSlidingWindow);

  for (int i = 0; i < nsliding; i++) {
    double charge = 0;
    int sample_start = i * fSlidingWindow;
    int sample_end = (i + 1) * fSlidingWindow;
    for (int j = sample_start; j < sample_end; j++) {
      double voltage = DigitToVoltage(fDigitWfm[j]);
      charge += (-voltage * fTimeStep) / fTermOhms;  // in pC
    }
    if (charge > fChargeThresh) {
      fTotalCharge += charge;
    }
  }
}

double SingleLognormal(double* x, double* par) {
  /*
  Lognormal distribution
  */

  double mag = par[0];
  double theta = par[1];
  double baseline = par[2];

  double m = par[3];
  double s = par[4];

  if (x[0] <= theta) {
    return baseline;
  }

  double q = baseline - mag * TMath::LogNormal(x[0], s, theta, m);
  return q;
}

void WaveformAnalysis::FitWaveform() {
  /*
  Fit the PMT pulse to a lognormal distribution
  */
  // Fit around the peak
  TH1D* wfm = new TH1D("wfm", "wfm", fDigitWfm.size(), 0, fDigitWfm.size() * fTimeStep);
  for (UShort_t i = 0; i < fDigitWfm.size(); i++) {
    wfm->SetBinContent(i, DigitToVoltage(fDigitWfm[i]));
    // Arb. choice, TODO
    wfm->SetBinError(i, fVoltageRes * 2.0);
  }
  double bf = fDigitTime - fFitWindowLow;
  double tf = fDigitTime + fFitWindowHigh;

  // Check the fit range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf > fDigitWfm.size() * fTimeStep) ? fDigitWfm.size() * fTimeStep : tf;

  // Check the timing range is within the digitizer window
  double thigh = fDigitTime + fFitScale + fFitWindowHigh;
  thigh = (thigh > fDigitWfm.size() * fTimeStep) ? fDigitWfm.size() * fTimeStep : thigh;

  double tmed = fDigitTime - fFitScale;
  tmed = (tmed > 0) ? tmed : 0;

  double tlow = fDigitTime - fFitScale - fFitWindowLow;
  tlow = (tlow > 0) ? tlow : 0;

  const int ndf = 5;
  TF1* ln_fit = new TF1("ln_fit", SingleLognormal, bf, tf, ndf);
  ln_fit->SetParameter(0, 40.0);
  // Fit assumes SPE waveform of limited size
  ln_fit->SetParLimits(0, 1.0, 400.0);
  // Fitted time around the peak
  ln_fit->SetParameter(1, tmed);
  ln_fit->SetParLimits(1, tlow, thigh);
  // Baseline centered around zero
  ln_fit->SetParameter(2, 0.0);
  ln_fit->SetParLimits(2, -1.0, 1.0);
  ln_fit->SetParameter(3, fFitScale);
  ln_fit->SetParLimits(3, fFitScale - 5.0, fFitScale + 5.0);

  ln_fit->FixParameter(4, fFitShape);

  wfm->Fit("ln_fit", "0QR", "", bf, tf);

  fFittedHeight = ln_fit->GetParameter(0);
  fFittedTime = ln_fit->GetParameter(1) + ln_fit->GetParameter(3);
  fFittedBaseline = ln_fit->GetParameter(2);
  fChi2NDF = ln_fit->GetChisquare() / ln_fit->GetNDF();

  delete wfm;
  delete ln_fit;
}

Processor::Result WaveformAnalysis::Event(DS::Root* ds, DS::EV* ev) {
  if (!ev->DigitizerExists()) {
    warn << "Running waveform analysis, but no digitzer information." << newline;
    return Processor::Result::OK;
  }
  DS::Digit* dsdigit = &ev->GetDigitizer();
  DS::Run* run = DS::RunStore::GetRun(ds->GetRunID());
  const DS::ChannelStatus* ch_status = run->GetChannelStatus();
  std::vector<int> pmt_ids = dsdigit->GetIDs();
  double total_charge = 0;
  for (int pmt_id : pmt_ids) {
    // Do not analyze negative pmtid channels, since they do not correspond to real PMTs.
    if (pmt_id < 0) continue;
    if (!ch_status->GetOnlineByPMTID(pmt_id)) continue;
    DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(pmt_id);
    double time_offset = fApplyCableOffset ? ch_status->GetCableOffsetByPMTID(pmt_id) : 0.0;
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
