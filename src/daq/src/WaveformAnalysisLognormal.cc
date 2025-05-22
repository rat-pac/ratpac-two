#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisLognormal.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {
void WaveformAnalysisLognormal::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
    fFitShape = fDigit->GetD("lognormal_shape");
    fFitScale = fDigit->GetD("lognormal_scale");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisLognormal: Unable to find analysis parameters.");
  }
}

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

void WaveformAnalysisLognormal::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisLognormal: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);
  fDigitTimeInWindow = digitpmt->GetDigitizedTimeNoOffset();
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
  double bf = fDigitTimeInWindow - fFitWindowLow;
  double tf = fDigitTimeInWindow + fFitWindowHigh;

  // Check the fit range is within the digitizer window
  bf = (bf > 0) ? bf : 0;
  tf = (tf > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : tf;

  // Check the timing range is within the digitizer window
  double thigh = fDigitTimeInWindow + fFitScale + fFitWindowHigh;
  thigh = (thigh > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : thigh;

  double tmed = fDigitTimeInWindow - fFitScale;
  tmed = (tmed > 0) ? tmed : 0;

  double tlow = fDigitTimeInWindow - fFitScale - fFitWindowLow;
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

}  // namespace RAT
