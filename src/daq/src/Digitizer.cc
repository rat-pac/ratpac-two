#include <CLHEP/Random/RandGauss.h>

#include <RAT/Digitizer.hh>
#include <RAT/PMTWaveform.hh>

namespace RAT {

Digitizer::Digitizer(std::string digitName) { SetDigitizerType(digitName); }

void Digitizer::SetDigitizerType(std::string digitName) {
  fDigitName = digitName;
  fLdaq = DB::Get()->GetLink("DIGITIZER", fDigitName);

  fSamplingRate = fLdaq->GetD("sampling_rate");  // In GHz
  fOffset = fLdaq->GetD("offset");               // vertical offset in mV
  fVhigh = fLdaq->GetD("volt_high");             // in mV
  fVlow = fLdaq->GetD("volt_low");               // in mV
  fNBits = fLdaq->GetI("nbits");
  fNoiseAmpl = fLdaq->GetD("noise_amplitude");  // digitizer noise, in mV
  fNSamples = fLdaq->GetD("nsamples");

  fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS");
  fPedWindowLow = fDigit->GetI("pedestal_window_low");
  fPedWindowHigh = fDigit->GetI("pedestal_window_high");
  fLookback = fDigit->GetI("lookback");
  fIntWindowLow = fDigit->GetD("integration_window_low");
  fIntWindowHigh = fDigit->GetD("integration_window_high");
  fConstFrac = fDigit->GetD("constant_fraction");

  detail << dformat("  Digitizer: Channel Noise: ............ %6.2f adc counts\n", fNoiseAmpl);
  detail << dformat("  Digitizer: Sampling Rate: ............ %6.2f ns\n", fSamplingRate);
  detail << dformat("  Digitizer: Total Number of Samples: .. %d \n", fNSamples);
  detail << dformat("  Digitizer: Voltage offset: ........... %6.2f mV\n", fOffset);
  detail << dformat("  Digitizer: Voltage High: ............. %6.2f mV\n", fVhigh);
  detail << dformat("  Digitizer: Voltage Low: .............. %6.2f mV\n", fVlow);
}

// Add channel to digitizer and immdediatly digitize analogue waveform
void Digitizer::AddChannel(int ichannel, PMTWaveform pmtwf) {
  // Reset
  fDigitWaveForm[ichannel].clear();

  double timeres = 1.0 / fSamplingRate;  // in ns
  int nADCs = 1 << fNBits;
  double adcpervolt = nADCs / (fVhigh - fVlow);

  // Second digitize analogue
  double currenttime = 0;
  for (int isample = 0; isample < fNSamples; isample++) {
    double voltage = pmtwf.GetHeight(currenttime);
    voltage += fNoiseAmpl * CLHEP::RandGauss::shoot();           // add electronic noise
    int adcs = round((voltage - fVlow + fOffset) * adcpervolt);  // digitize: V->ADC

    // Manage voltage saturation
    if (adcs < 0) {
      adcs = 0;
    } else if (adcs >= nADCs) {
      adcs = nADCs - 1;
    }

    // Save sample
    fDigitWaveForm[UShort_t(ichannel)].push_back(UShort_t(adcs));

    // Step on time
    currenttime += timeres;
  }
}
}  // namespace RAT
