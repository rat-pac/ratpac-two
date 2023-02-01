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
}

void WaveformAnalysis::RunAnalysis(DS::PMT *pmt, int pmtID, Digitizer *fDigitizer){

  double dy = (fDigitizer->fVhigh - fDigitizer->fVlow) / (pow(2, fDigitizer->fNBits));

  double digit_time = CalculateTime(fDigitizer->fDigitWaveForm[pmtID], fPedWindowLow,
                                    fPedWindowHigh, dy, fDigitizer->fSamplingRate,
                                    fConstFrac, fLookback);

  double time_step = 1.0 / fDigitizer->fSamplingRate;
  int low_int_window = int((digit_time - fIntWindowLow) / time_step);
  int high_int_window = int((digit_time + fIntWindowHigh) / time_step);

  double digit_charge = Integrate(fDigitizer->fDigitWaveForm[pmtID], fPedWindowLow,
                                  fPedWindowHigh, dy, fDigitizer->fSamplingRate,
                                  low_int_window, high_int_window, fDigitizer->fTerminationOhms);

  pmt->SetDigitizedTime(digit_time);
  pmt->SetDigitizedCharge(digit_charge);
}

double WaveformAnalysis::CalculatePedestal(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window) {
  /*
  Calculate the baseline in the window between low - high samples.
  */
  double pedestal = 0;

  if(low_window > wfm.size()){
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than waveform size.");
  }
  else if(low_window > high_window){
    Log::Die("WaveformAnalysis: Start of pedestal window must be smaller than end of pedestal window.");
  }

  // Ensure end of pedestal window is less than waveform size
  high_window = (high_window > wfm.size()) ? wfm.size() : high_window;

  for (UShort_t i = low_window; i < high_window; i++) {
    pedestal += wfm[i];
  }
  pedestal /= (high_window - low_window);

  return pedestal;
}

double WaveformAnalysis::Interpolate(double voltage1, double voltage2, double voltage_crossing, double time_step) {
  /*
  Linearly interpolate between two samples
  */
  double deltav = (voltage1 - voltage2);
  double dx = (voltage_crossing - voltage2) / deltav;
  double dt = dx * time_step;

  return dt;
}

void WaveformAnalysis::GetPeak(std::vector<UShort_t> wfm, double dy, double pedestal, double &peak,
                               UShort_t &peak_sample) {
  /*
  Calculate the peak (in mV) and the corresponding sample.
  */
  for (size_t i = 0; i < wfm.size(); i++) {
    double voltage = (wfm[i] - pedestal) * dy;

    // Downward going pulse
    if (voltage < peak) {
      peak = voltage;
      peak_sample = i;
    }
  }
}

UShort_t WaveformAnalysis::GetThresholdCrossing(std::vector<UShort_t> wfm, double dy, double pedestal, double peak,
                                                UShort_t peak_sample, double cfd_fraction, UShort_t lookback) {
  /*
  Identifies the sample at which the constant-fraction threshold crossing occurs
   */
  UShort_t threshold_crossing_sample = 0;
  double voltage_crossing = cfd_fraction * peak;

  // Make sure we don't scan passed the beginning of the waveform 
  Int_t lb = Int_t(peak_sample) - Int_t(lookback);
  UShort_t back_window = (lb > 0) ? lb : 0;

  // Start at the peak and scan backwards
  for (UShort_t i = peak_sample; i > back_window; i--) {
    double voltage = (wfm[i] - pedestal) * dy;

    if (voltage > voltage_crossing) {
      threshold_crossing_sample = i;
      break;
    }

    // Reached the begining of the waveform
    // returned an invalid value
    if (i == 0) return INVALID;
  }

  return threshold_crossing_sample;
}

double WaveformAnalysis::CalculateTime(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window, double dy,
                                       double sampling_rate, double cfd_fraction, UShort_t lookback) {
  /*
  Apply constant-fraction discriminator to digitized PMT waveforms.
  */
  double pedestal = CalculatePedestal(wfm, low_window, high_window);

  // Calculate peak in mV
  double peak = 9999;
  UShort_t peak_sample = 0;
  GetPeak(wfm, dy, pedestal, peak, peak_sample);

  UShort_t threshold_crossing_sample = GetThresholdCrossing(wfm, dy, pedestal, peak, peak_sample, cfd_fraction, lookback);

  if(threshold_crossing_sample == INVALID || 
     threshold_crossing_sample >= wfm.size()){
    return INVALID;
  }

  double time_step = 1.0 / sampling_rate;  // in ns

  // Interpolate between the two samples where the CFD threshold is crossed
  double voltage_crossing = cfd_fraction * peak;
  double v1 = (wfm[threshold_crossing_sample + 1] - pedestal) * dy;
  double v2 = (wfm[threshold_crossing_sample] - pedestal) * dy;
  double dt = Interpolate(v1, v2, voltage_crossing, time_step);

  double tcdf = double(threshold_crossing_sample) * time_step + dt;

  return tcdf;
}

double WaveformAnalysis::Integrate(std::vector<UShort_t> wfm, UShort_t low_window, UShort_t high_window, double dy,
                                   double sampling_rate, int integration_window_low, int integration_window_high,
                                   double termination_ohms) {
  /*
  Integrate the digitized waveform to calculate charge
  */
  double pedestal = CalculatePedestal(wfm, low_window, high_window);
  double time_step = 1.0 / sampling_rate;  // in ns
  double charge = 0;

  if(integration_window_low >= wfm.size()){
    return INVALID;
  }

  // Make sure not to integrate past the end of the waveform
  integration_window_high = (integration_window_high > wfm.size()) ? wfm.size() : integration_window_high;
  // Make sure not to integrate before the waveform starts
  integration_window_low = (integration_window_low < 0) ? 0 : integration_window_low;

  for (int i = integration_window_low; i < integration_window_high; i++) {
    double voltage = (wfm[i] - pedestal) * dy;
    charge += (-voltage * time_step) / termination_ohms;  // in pC
  }

  return charge;
}

}  // namespace RAT
