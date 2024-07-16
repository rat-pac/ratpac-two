#include <RAT/PMTWaveform.hh>
#include <iostream>

namespace RAT {

PMTWaveform::PMTWaveform() {}

PMTWaveform::~PMTWaveform() {}

double PMTWaveform::GetHeight(double currenttime) {
  double height = 0.;
  for (unsigned int i = 0; i < fPulse.size(); i++) {
    height += fPulse[i].GetPulseHeight(currenttime);
  }
  return height;
}

}  // namespace RAT
