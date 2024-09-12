#include <CLHEP/Random/RandGauss.h>

#include <RAT/Digitizer.hh>

namespace RAT {

Digitizer::Digitizer(std::string digitName) { SetDigitizerType(digitName); }

void Digitizer::SetDigitizerType(std::string digitName) {
  fDigitName = digitName;
  fLdaq = DB::Get()->GetLink("DIGITIZER", fDigitName);

  fSamplingRate = fLdaq->GetD("sampling_rate");  // In GHz
  fNBits = fLdaq->GetI("nbits");
  fNSamples = fLdaq->GetI("nsamples");
  fOffset = fLdaq->GetD("offset");              // vertical offset in mV
  fVhigh = fLdaq->GetD("volt_high");            // in mV
  fVlow = fLdaq->GetD("volt_low");              // in mV
  fNoiseAmpl = fLdaq->GetD("noise_amplitude");  // digitizer noise, in mV
  fTerminationOhms = fLdaq->GetD("termination_ohms");

  detail << dformat("  Digitizer: Channel Noise: ............ %6.2f adc counts\n", fNoiseAmpl);
  detail << dformat("  Digitizer: Sampling Rate: ............ %6.2f ns\n", fSamplingRate);
  detail << dformat("  Digitizer: Total Number of Samples: .. %d \n", fNSamples);
  detail << dformat("  Digitizer: Voltage offset: ........... %6.2f mV\n", fOffset);
  detail << dformat("  Digitizer: Voltage High: ............. %6.2f mV\n", fVhigh);
  detail << dformat("  Digitizer: Voltage Low: .............. %6.2f mV\n", fVlow);
}

void Digitizer::AddWaveformGenerator(std::string modelName) {
  fPMTWaveformGenerators[modelName] = new PMTWaveformGenerator(modelName);
}

void Digitizer::DigitizePMT(DS::MCPMT* mcpmt, int pmtID, double triggerTime, DS::PMTInfo* pmtinfo) {
  PMTWaveform pmtwfm = fPMTWaveformGenerators[pmtinfo->GetModelNameByID(pmtID)]->GenerateWaveforms(mcpmt, triggerTime);
  AddChannel(pmtID, pmtwfm);
}

void Digitizer::ClearWaveforms() { fDigitWaveForm.clear(); }

void Digitizer::WriteToEvent(DS::EV* ev) {
  DS::Digit digit;

  std::map<int, std::vector<UShort_t>> waveforms = fDigitWaveForm;
  for (std::map<int, std::vector<UShort_t>>::const_iterator it = waveforms.begin(); it != waveforms.end(); it++) {
    digit.SetWaveform(it->first, waveforms[it->first]);
  }

  digit.SetDigitName(fDigitName);
  digit.SetNSamples(uint32_t(fNSamples));
  digit.SetNBits(UShort_t(fNBits));
  digit.SetDynamicRange((fVhigh - fVlow));
  digit.SetSamplingRate(fSamplingRate);
  digit.SetTerminationOhms(fTerminationOhms);

  ev->SetDigitizer(digit);
  ClearWaveforms();
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
    fDigitWaveForm[ichannel].push_back(UShort_t(adcs));

    // Step on time
    currenttime += timeres;
  }
}

// Set the digitizer parameters
void Digitizer::SetI(std::string param, int value) {
  if (param == "nbits")
    fNBits = value;
  else if (param == "nsamples")
    fNSamples = value;
  else
    throw Processor::ParamUnknown(param);
}

// Set the digitizer parameters
void Digitizer::SetD(std::string param, double value) {
  if (param == "sampling_rate")
    fSamplingRate = value;
  else if (param == "offset")
    fOffset = value;
  else if (param == "volt_high")
    fVhigh = value;
  else if (param == "volt_low")
    fVlow = value;
  else if (param == "noise_amplitude")
    fNoiseAmpl = value;
  else if (param == "termination_ohms")
    fTerminationOhms = value;
  else
    throw Processor::ParamUnknown(param);
}

}  // namespace RAT
