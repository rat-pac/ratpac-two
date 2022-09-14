#include <CLHEP/Random/RandGauss.h>

#include <RAT/Log.hh>
#include <RAT/PMTPulse.hh>
#include <RAT/PMTWaveform.hh>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace RAT {

PMTWaveform::PMTWaveform() {}

PMTWaveform::~PMTWaveform() {}

double PMTWaveform::GetHeight(double currenttime) {
  float height = 0.;
  unsigned int i = 0;
  while (i < fPulse.size() && fPulse[i]->GetPulseStartTime() <= currenttime) {
    height += fPulse[i]->GetPulseHeight(currenttime);
    i++;
  }
  return height;
}

}  // namespace RAT
