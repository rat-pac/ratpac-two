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

double PMTPulse::GetDataDrivenPulseVal(double time) {
  if ((time < fPulseTimes[0]) || (time > fPulseTimes[fPulseTimes.size() - 1])) {
    // pulse not defined for this value
    return 0.;
  }
  size_t i = 0;
  for (i = 0; i < fPulseTimes.size() - 1; i++) {
    if (time > fPulseTimes[i]) {
      continue;
    } else if (time == fPulseTimes[i]) {
      return fPulseValues[i];
    } else {
      break;
    }
  }
  // interpolate between points
  return fPulseValues[i - 1] +
         (fPulseValues[i] - fPulseValues[i - 1]) * (time - fPulseTimes[i - 1]) / (fPulseTimes[i] - fPulseTimes[i - 1]);
}

double PMTPulse::GetPulseHeight(double utime) {
  double start_time = fStartTime + fPulseTimeOffset;
  double delta_t = utime - start_time;

  double val = 0.0;
  if (fPulseType == "analytic") {
    if (fPulseShape == "lognormal") {
      if (delta_t > -fLogNPulseMean) {
        val = TMath::LogNormal(delta_t, fLogNPulseWidth, -fLogNPulseMean, fLogNPulseMean);
      }
    } else if (fPulseShape == "gaussian") {
      val = TMath::Gaus(delta_t, 0., fGausPulseWidth, kTRUE);
    }
  } else if (fPulseType == "datadriven") {
    if (delta_t >= fPulseTimes[0]) {
      val = GetDataDrivenPulseVal(delta_t);
    }
  }

  double height = fPulseOffset + fPulsePolaritySign * (fPulseCharge * val);

  if (abs(height) < fPulseMin) {
    height = 0;
  }

  return height;
}

}  // namespace RAT
