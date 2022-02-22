#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>
#include <RAT/PMTWaveform.hh>
#include <RAT/PMTPulse.hh>
#include <RAT/Log.hh>
#include <CLHEP/Random/RandGauss.h>

namespace RAT {

PMTWaveform::PMTWaveform()
{
}

PMTWaveform::~PMTWaveform()
{
}

double PMTWaveform::GetHeight(double currenttime){
  float height = 0.;
  unsigned int i = 0;
  while (i<fPulse.size() && fPulse[i]->GetPulseStartTime()<=currenttime){
    height+=fPulse[i]->GetPulseHeight(currenttime);
    i++;
  }
  return height;
}

} // namespace RAT
