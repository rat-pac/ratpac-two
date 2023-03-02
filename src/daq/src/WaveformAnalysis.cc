#include <RAT/WaveformAnalysis.hh>
#include <RAT/Log.hh>
#include <iostream>

namespace RAT {

WaveformAnalysis::WaveformAnalysis(){

  fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS");
  fPedWindowLow = fDigit->GetI("pedestal_window_low");
  fPedWindowHigh = fDigit->GetI("pedestal_window_high");
  fLookback = fDigit->GetI("lookback");
  fIntWindowLow = fDigit->GetD("integration_window_low");
  fIntWindowHigh = fDigit->GetD("integration_window_high");
  fConstFrac = fDigit->GetD("constant_fraction");
  fThreshold = fDigit->GetD("voltage_threshold");
}

void WaveformAnalysis::RunAnalysis(DS::PMT *pmt, int pmtID, Digitizer *fDigitizer){

  fVoltageRes = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));
  fTimeStep = 1.0 / fDigitizer->fSamplingRate;  // in ns

  fDigitWfm = fDigitizer->fDigitWaveForm[pmtID]; 

  CalculatePedestal();

  double digit_time = CalculateTime();

  fLowIntWindow = int((digit_time - fIntWindowLow) / fTimeStep);
  fHighIntWindow = int((digit_time + fIntWindowHigh) / fTimeStep);
  fTermOhms = fDigitizer->fTerminationOhms;

  double digit_charge = Integrate();

  pmt->SetDigitizedTime(digit_time);
  pmt->SetDigitizedCharge(digit_charge);
  pmt->SetInterpolatedTime(fInterpolatedTime);
  pmt->SetSampleTime(fThresholdCrossing);
  pmt->SetNCrossings(fNCrossings);
  pmt->SetPedestal(fPedestal);
}

void WaveformAnalysis::CalculatePedestal() {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  fPedestal = 0;

  if(fPedWindowLow > fDigitWfm.size()){
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than waveform size.");
  }
  else if(fPedWindowLow > fPedWindowHigh){
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than end of pedestal window.");
  }

  // Ensure end of pedestal window is less than waveform size
  fPedWindowHigh = (fPedWindowHigh > fDigitWfm.size()) ? fDigitWfm.size() : fPedWindowHigh;

  for (UShort_t i = fPedWindowLow; i < fPedWindowHigh; i++) {
    fPedestal += fDigitWfm[i];
  }
  fPedestal /= (fPedWindowHigh - fPedWindowLow);
}

void WaveformAnalysis::Interpolate(double voltage1, double voltage2){
  /*
  Linearly interpolate between two samples
  */
  double deltav = (voltage1 - voltage2);
  double dx = (fVoltageCrossing - voltage2) / deltav;
  fInterpolatedTime = dx * fTimeStep;
}

void WaveformAnalysis::GetPeak(){
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  fVoltagePeak = 999;
  fSamplePeak = -999;
  for (size_t i = 0; i < fDigitWfm.size(); i++) {
    double voltage = (fDigitWfm[i] - fPedestal) * fVoltageRes;

    // Downward going pulse
    if (voltage < fVoltagePeak) {
      fVoltagePeak = voltage;
      fSamplePeak = i;
    }
  }
}

void WaveformAnalysis::GetThresholdCrossing(){
  /*
  Identifies the sample at which the constant-fraction threshold crossing occurs
   */
  fThresholdCrossing = 0;
  fVoltageCrossing = fConstFrac * fVoltagePeak;

  // Make sure we don't scan passed the beginning of the waveform 
  Int_t lb = Int_t(fSamplePeak) - Int_t(fLookback);
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
    if (i == 0){
      fThresholdCrossing = INVALID;
      break;
    }
  }
}

void WaveformAnalysis::GetNCrossings(){
  /*
  Calculates the total number of threshold crossings
  */
  fNCrossings = 0;
  fTimeOverThreshold = 0;

  bool fCrossed = false;
  // Start at the peak and scan backwards
  for (UShort_t i = 0; i > fDigitWfm.size(); i--) {
    double voltage = (fDigitWfm[i] - fPedestal) * fVoltageRes;

    if (voltage < fThreshold) {
      if(!fCrossed){
        fNCrossings += 1;
      }
      fTimeOverThreshold += fTimeStep;
      fCrossed = true;
    }

    if (voltage < fThreshold) {
      fCrossed = false;
    }
  }
}

double WaveformAnalysis::CalculateTime() {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */

  // Calculate peak in mV
  GetPeak();

  // Get the sample where the voltage thresh is crossed
  GetThresholdCrossing();

  GetNCrossings();

  if(fThresholdCrossing == INVALID || 
     fThresholdCrossing >= fDigitWfm.size()){
    return INVALID;
  }

  if(fThresholdCrossing >= fDigitWfm.size()){
    Log::Die("WaveformAnalysis: Threshold crossing sample larger than waveform window."); 
  }

  // Interpolate between the two samples where the CFD threshold is crossed
  double v1 = (fDigitWfm[fThresholdCrossing + 1] - fPedestal) * fVoltageRes;
  double v2 = (fDigitWfm[fThresholdCrossing] - fPedestal) * fVoltageRes;
  Interpolate(v1, v2);

  double tcdf = double(fThresholdCrossing) * fTimeStep + fInterpolatedTime;

  return tcdf;
}

double WaveformAnalysis::Integrate() {
  /*
  Integrate the digitized waveform to calculate charge
  */
  double charge = 0;

  if(fLowIntWindow >= fDigitWfm.size()){
    return INVALID;
  }

  // Make sure not to integrate past the end of the waveform
  fHighIntWindow = (fHighIntWindow > fDigitWfm.size()) ? fDigitWfm.size() : fHighIntWindow;
  // Make sure not to integrate before the waveform starts
  fLowIntWindow = (fLowIntWindow < 0) ? 0 : fLowIntWindow;

  for (int i = fLowIntWindow; i < fHighIntWindow; i++) {
    double voltage = (fDigitWfm[i] - fPedestal) * fVoltageRes;
    charge += (-voltage * fTimeStep) / fTermOhms;  // in pC
  }

  return charge;
}

}  // namespace RAT
