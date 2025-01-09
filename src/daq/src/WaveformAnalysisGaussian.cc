#include <TF1.h>
#include <TH1D.h>
#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/WaveformAnalysisGaussian.hh>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {
void WaveformAnalysisGaussian::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    fFitWindowLow = fDigit->GetD("fit_window_low");
    fFitWindowHigh = fDigit->GetD("fit_window_high");
    fFitWidth = fDigit->GetD("gaussian_width");
    fFitWidthLow = fDigit->GetD("gaussian_width_low");
    fFitWidthHigh = fDigit->GetD("gaussian_width_high");
  } catch (DBNotFoundError) {
    RAT::Log::Die("WaveformAnalysisGaussian: Unable to find analysis parameters.");
  }
}

void WaveformAnalysisGaussian::SetD(std::string param, double value) {
  if (param == "fit_window_low") {
    fFitWindowLow = value;
  } else if (param == "fit_window_high") {
    fFitWindowHigh = value;
  } else if (param == "gaussian_width") {
    fFitWidth = value;
  } else if (param == "gaussian_width_low") {
    fFitWidthLow = value;
  } else if (param == "gaussian_width_high") {
    fFitWidthHigh = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisGaussian::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisGaussian: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal = pedestal);
  fDigitTimeInWindow = digitpmt->GetDigitizedTimeNoOffset();
  // Fit waveform to lognormal
  FitWaveform(voltWfm);

  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("Gaussian");
  fit_result->AddPE(fFittedTime, fFittedCharge,
                    {{"baseline", fFittedBaseline}, {"width", fFittedWidth}, {"chi2ndf", fChi2NDF}});
}

static double SingleGaussian(double* x, double* par) {
  /*
  Normal distribution
  */

  double mag = par[0];
  double mu = par[1];
  double baseline = par[2];
  double s = par[3];

  double q = baseline - mag * TMath::Gaus(x[0], mu, s, kTRUE);
  return q;
}

void WaveformAnalysisGaussian::FitWaveform(const std::vector<double>& voltWfm) {
  /*
  Fit the PMT pulse to a gaussian distribution
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
  double thigh = fDigitTimeInWindow + fFitWindowHigh;
  thigh = (thigh > voltWfm.size() * fTimeStep) ? voltWfm.size() * fTimeStep : thigh;

  double tmed = fDigitTimeInWindow;
  tmed = (tmed > 0) ? tmed : 0;

  double tlow = fDigitTimeInWindow - fFitWindowLow;
  tlow = (tlow > 0) ? tlow : 0;

  const int ndf = 4;
  TF1* gaus_fit = new TF1("gaus_fit", SingleGaussian, bf, tf, ndf);
  gaus_fit->SetParameter(0, 40.0);
  // Fit assumes SPE waveform of limited size
  gaus_fit->SetParLimits(0, 1.0, 400.0);
  // Fitted time around the peak
  gaus_fit->SetParameter(1, tmed);
  gaus_fit->SetParLimits(1, tlow, thigh);
  // Baseline centered around zero
  gaus_fit->SetParameter(2, 0.0);
  gaus_fit->SetParLimits(2, -1.0, 1.0);
  gaus_fit->SetParameter(3, fFitWidth);
  gaus_fit->SetParLimits(3, fFitWidthLow, fFitWidthHigh);

  wfm->Fit("gaus_fit", "0QR", "", bf, tf);

  fFittedCharge = gaus_fit->GetParameter(0) / fTermOhms;
  fFittedTime = gaus_fit->GetParameter(1);
  fFittedBaseline = gaus_fit->GetParameter(2);
  fFittedWidth = gaus_fit->GetParameter(3);
  fChi2NDF = gaus_fit->GetChisquare() / gaus_fit->GetNDF();

  delete wfm;
  delete gaus_fit;
}

}  // namespace RAT
