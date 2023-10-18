#include <TMath.h>

#include <RAT/Log.hh>
#include <RAT/PMTPulse.hh>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace RAT {

PMTPulse::PMTPulse(std::string pulseType, std::string pulseShape) {
  fPulseType = pulseType;
  fPulseShape = pulseShape;
}

double PMTPulse::GetPulseHeight(double utime) {
  double height = 0.0;
  double start_time = fStartTime + fPulseTimeOffset;
  double delta_t = utime - start_time;

  if (delta_t > 0.0) {
    double val = 0.0;
    if (fPulseType == "analytic") {
      if (fPulseShape == "lognormal") {
        val = TMath::LogNormal(delta_t, fPulseWidth, 0., fPulseMean);
      }
    } else if (fPulseType == "datadriven") {
      int i = 0;
      for (i = 0; i < fPulseTimes.size() - 1; i++) {
        if (delta_t > fPulseTimes[i]) {
          continue;
        } else if (delta_t == fPulseTimes[i]) {
          return fPulseValues[i];
        } else {
          break;
        }
      }
      val = fPulseValues[i - 1] + (fPulseValues[i] - fPulseValues[i - 1]) * (delta_t - fPulseTimes[i - 1]) /
                                      (fPulseTimes[i] - fPulseTimes[i - 1]);
    }
    height = fPulseOffset + fPulsePolaritySign * (fPulseCharge * val);
  }
  if (abs(height) < fPulseMin) {
    height = 0;
  }

  return height;
}

}  // namespace RAT
