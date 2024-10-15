#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisLognormal.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/RunStore.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

WaveformAnalysisLognormal::WaveformAnalysisLognormal() : WaveformAnalysisLognormal::WaveformAnalysisLognormal("") {}

WaveformAnalysisLognormal::WaveformAnalysisLognormal(std::string analyzer_name)
    : Processor("WaveformAnalysisLognormal") {
  Configure(analyzer_name);
}

void WaveformAnalysisLognormal::Configure(const std::string& analyzer_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", analyzer_name);
    fPedWindowLow = fDigit->GetI("pedestal_window_low");
    fPedWindowHigh = fDigit->GetI("pedestal_window_high");
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
    fFitShape = fDigit->GetD("lognormal_shape");
    fFitScale = fDigit->GetD("lognormal_scale");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisLognormal: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisLognormal::SetS(std::string param, std::string value) {
  if (param == "analyzer_name") {
    Configure(value);
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisLognormal::SetI(std::string param, int value) { throw Processor::ParamUnknown(param); }

void WaveformAnalysisLognormal::SetD(std::string param, double value) {
  if (param == "fit_window_low") {
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

void WaveformAnalysisLognormal::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, Digitizer* fDigitizer) {
  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  std::vector<UShort_t> digitWfm = fDigitizer->fDigitWaveForm[pmtID];
  fTermOhms = fDigitizer->fTerminationOhms;
  DoAnalysis(digitpmt, digitWfm);
}

void WaveformAnalysisLognormal::RunAnalysis(DS::DigitPMT* digitpmt, int pmtID, DS::Digit* dsdigit) {
  fVoltageRes = dsdigit->GetVoltageResolution();
  fTimeStep = dsdigit->GetTimeStepNS();
  fTermOhms = dsdigit->GetTerminationOhms();
  std::vector<UShort_t> digitWfm = dsdigit->GetWaveform(pmtID);
  DoAnalysis(digitpmt, digitWfm);
}

void WaveformAnalysisLognormal::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisLognormal: Setting pedestal, run WaveformAnalysisCommon first.");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);
  fDigitTime = digitpmt->GetDigitizedTimeNoOffset();
  // Fit waveform to lognormal
  FitWaveform(voltWfm);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("Lognormal");
  fit_result->AddPE(fFittedTime, fFittedCharge, {{"baseline", fFittedBaseline}, {"chi2ndf", fChi2NDF}});
}

static double SingleLognormal1(double* x, double* par) {
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

void WaveformAnalysisLognormal::FitWaveform(const std::vector<double>& voltWfm) {
  /*
  Fit the PMT pulse to a lognormal distribution
  */
  // Fit around the peak
  TH1D* wfm = new TH1D("wfm", "wfm", voltWfm.size(), 0, voltWfm.size() * fTimeStep);
  for (UShort_t i = 0; i < voltWfm.size(); i++) {
    wfm->SetBinContent(i, voltWfm[i]);
    // Arb. choice, TODO
    wfm->SetBinError(i, fVoltageRes * 2.0);
  }
  double bf = fDigitTime - fFitWindowLow;
  double tf = fDigitTime + fFitWindowHigh;

  // Check the fit range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : tf;

  // Check the timing range is within the digitizer window
  double thigh = fDigitTime + fFitScale + fFitWindowHigh;
  thigh = (thigh > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : thigh;

  double tmed = fDigitTime - fFitScale;
  tmed = (tmed > 0) ? tmed : 0;

  double tlow = fDigitTime - fFitScale - fFitWindowLow;
  tlow = (tlow > 0) ? tlow : 0;

  const int ndf = 5;
  TF1* ln_fit = new TF1("ln_fit", SingleLognormal1, bf, tf, ndf);
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

  fFittedCharge = ln_fit->GetParameter(0) / fTermOhms;
  fFittedTime = ln_fit->GetParameter(1) + ln_fit->GetParameter(3);
  fFittedBaseline = ln_fit->GetParameter(2);
  fChi2NDF = ln_fit->GetChisquare() / ln_fit->GetNDF();

  delete wfm;
  delete ln_fit;
}

Processor::Result WaveformAnalysisLognormal::Event(DS::Root* ds, DS::EV* ev) {
  if (!ev->DigitizerExists()) {
    warn << "Running waveform analysis, but no digitzer information." << newline;
    return Processor::Result::OK;
  }
  DS::Digit* dsdigit = &ev->GetDigitizer();
  DS::Run* run = DS::RunStore::GetRun(ds->GetRunID());
  std::vector<int> pmt_ids = ev->GetAllDigitPMTIDs();
  for (int pmt_id : pmt_ids) {
    DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(pmt_id);
    RunAnalysis(digitpmt, pmt_id, dsdigit);
  }
  return Processor::Result::OK;
}

}  // namespace RAT
