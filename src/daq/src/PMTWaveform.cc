#include <RAT/PMTWaveform.hh>
#include <iostream>

namespace RAT {

PMTWaveform::PMTWaveform() {}

PMTWaveform::~PMTWaveform() {}

double PMTWaveform::GetHeight(double currenttime) {
  double height = 0.;
  for (unsigned int i = 0; i < fPulse.size() && fPulse[i].GetPulseStartTime() <= currenttime; i++) {
    height += fPulse[i].GetPulseHeight(currenttime);
  }
  return height;
}

}  // namespace RAT
