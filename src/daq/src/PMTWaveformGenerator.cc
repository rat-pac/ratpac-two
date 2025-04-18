#include <RAT/DS/RunStore.hh>
#include <RAT/PMTWaveformGenerator.hh>
#include <Randomize.hh>
#include <algorithm>
#include <iostream>

namespace RAT {

PMTWaveformGenerator::PMTWaveformGenerator(std::string modelName) {
  // PMT pulse model specification
  fModelName = modelName;
  try {
    lpulse = DB::Get()->GetLink("PMTPULSE", modelName);
    lpulse->GetS("index");
    info << "PMTWaveformGenerator: Found pulse table for " << modelName << "." << newline;
  } catch (DBNotFoundError &e) {
    info << "PMTWaveformGenerator: Could not find pulse table for " << modelName << ". Trying default." << newline;
    try {
      lpulse = DB::Get()->GetLink("PMTPULSE", "");
      lpulse->GetS("index");
      info << "PMTWaveformGenerator: Using default pulse table for " << modelName << "." << newline;
    } catch (DBNotFoundError &e) {
      Log::Die("PMTWaveformGenerator: No default pulse table found.");
    }
  }
  try {
    fPMTPulseType = lpulse->GetS("pulse_type");
    info << "PMTWaveformGenerator: Using " << fPMTPulseType << " pulse for " << modelName << "." << newline;
  } catch (DBNotFoundError &e) {
    info << "PMTWaveformGenerator: Could not find pulse type for " << modelName << ". Using analytic as default."
         << newline;
    fPMTPulseShape = "analytic";
  }

  fPMTPulseShape == "";
  if (fPMTPulseType == "analytic") {
    try {
      fPMTPulseShape = lpulse->GetS("pulse_shape");
      info << "PMTWaveformGenerator: Using " << fPMTPulseShape << " pulse for " << modelName << "." << newline;
    } catch (DBNotFoundError &e) {
      info << "PMTWaveformGenerator: Could not find pulse shape for " << modelName << ". Using lognormal as default."
           << newline;
      fPMTPulseShape = "lognormal";
    }
  }

  fPMTPulseOffset = lpulse->GetD("pulse_offset");
  fPMTPulseMin = lpulse->GetD("pulse_min");
  fTerminationOhms = lpulse->GetD("termination_ohms");
  fPMTPulsePolarity = lpulse->GetZ("pulse_polarity_negative");

  if (fPMTPulseType == "analytic") {
    if (fPMTPulseShape == "lognormal") {
      fLogNPulseWidth = lpulse->GetD("lognormal_width");
      fLogNPulseMean = lpulse->GetD("lognormal_mean");
    } else if (fPMTPulseShape == "gaussian") {
      // 1D PDF of widths for pulses modeled as gaussians
      fGausPulseWidth = lpulse->GetDArray("gaussian_width");
      fGausPulseWidthProb = lpulse->GetDArray("gaussian_width_prob");

      if (fGausPulseWidth.size() != fGausPulseWidthProb.size()) {
        Log::Die("PMTWaveformGenerator: Widths and probability arrays are different in length.");
      }
      if (fGausPulseWidth.size() < 2) {
        Log::Die("PMTWaveformGenerator: Cannot define a PDF with fewer than 2 points.");
      }

      // set up PDF
      double integral = 0.0;
      fGausPulseWidthProbCumu = std::vector<double>(fGausPulseWidth.size());
      fGausPulseWidthProbCumu[0] = 0.0;
      for (size_t i = 0; i < fGausPulseWidth.size() - 1; i++) {
        // trapezoidal integration
        integral +=
            (fGausPulseWidth[i + 1] - fGausPulseWidth[i]) * (fGausPulseWidthProb[i] + fGausPulseWidthProb[i + 1]) / 2.0;
        fGausPulseWidthProbCumu[i + 1] = integral;
      }
      for (size_t i = 0; i < fGausPulseWidth.size(); i++) {
        // normalize PDF
        fGausPulseWidthProb[i] /= integral;
        fGausPulseWidthProbCumu[i] /= integral;
      }
    }
  }
  if (fPMTPulseType == "datadriven") {
    fPMTPulseShapeTimes = lpulse->GetDArray("pulse_shape_times");
    fPMTPulseShapeValues = lpulse->GetDArray("pulse_shape_values");
  }
}

PMTWaveformGenerator::~PMTWaveformGenerator() {}

double PMTWaveformGenerator::PickGaussianWidth() {
  // choose pulse width from PDF
  double rval = G4UniformRand();
  for (size_t i = 1; i < fGausPulseWidth.size(); i++) {
    if (rval <= fGausPulseWidthProbCumu[i]) {
      double dwidth = (fGausPulseWidth[i] - fGausPulseWidth[i - 1]);
      double dprob = (fGausPulseWidthProbCumu[i] - fGausPulseWidthProbCumu[i - 1]);
      // linear interpolation
      return (rval - fGausPulseWidthProbCumu[i - 1]) * dwidth / dprob + fGausPulseWidth[i - 1];
    }
  }
  return fGausPulseWidth[fGausPulseWidth.size() - 1];
}

PMTWaveform PMTWaveformGenerator::GenerateWaveforms(DS::MCPMT *mcpmt, double triggerTime) {
  PMTWaveform pmtwf;

  // Loop over PEs and create a pulse for each one
  for (int iph = 0; iph < mcpmt->GetMCPhotonCount(); iph++) {
    DS::MCPhoton *mcpe = mcpmt->GetMCPhoton(iph);
    double time_offset = DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetCableOffsetByPMTID(mcpmt->GetID());

    pmtwf.fPulse.push_back(PMTPulse(fPMTPulseType, fPMTPulseShape));
    PMTPulse *pmtpulse = &pmtwf.fPulse.back();
    pmtpulse->SetPulseCharge(mcpe->GetCharge() * fTerminationOhms);
    pmtpulse->SetPulseMin(fPMTPulseMin);
    pmtpulse->SetPulseOffset(fPMTPulseOffset);
    pmtpulse->SetPulseTimeOffset(time_offset);
    pmtpulse->SetPulseStartTime(mcpe->GetFrontEndTime() - triggerTime);
    pmtpulse->SetPulsePolarity(fPMTPulsePolarity);

    // Optional calibration parameter that scales the width of pulses for
    // individual channels, with a default value of 1.
    double pulse_width_scale =
        DS::RunStore::GetCurrentRun()->GetChannelStatus()->GetPulseWidthScaleByPMTID(mcpmt->GetID());

    if (fPMTPulseType == "analytic") {
      if (fPMTPulseShape == "lognormal") {
        pmtpulse->SetLogNPulseWidth(fLogNPulseWidth);
        pmtpulse->SetLogNPulseMean(pulse_width_scale * fLogNPulseMean);
      } else if (fPMTPulseShape == "gaussian") {
        pmtpulse->SetGausPulseWidth(pulse_width_scale * PickGaussianWidth());
      }
    } else if (fPMTPulseType == "datadriven") {
      std::vector<double> newPulseTimes(fPMTPulseShapeTimes.size());
      std::vector<double> newPulseValues(fPMTPulseShapeValues.size());
      // scale pulse width
      std::transform(fPMTPulseShapeTimes.begin(), fPMTPulseShapeTimes.end(), newPulseTimes.begin(),
                     [pulse_width_scale](double t) { return t * pulse_width_scale; });
      // re-normalize pulse shape
      double integral = 0.0;
      for (size_t i = 0; i < newPulseTimes.size() - 1; i++) {
        // trapezoidal integration
        integral +=
            (newPulseTimes[i + 1] - newPulseTimes[i]) * (fPMTPulseShapeValues[i] + fPMTPulseShapeValues[i + 1]) / 2.0;
      }
      for (size_t i = 0; i < newPulseValues.size(); i++) {
        newPulseValues[i] /= integral;
      }
      pmtpulse->SetPulseShapeTimes(newPulseTimes);
      pmtpulse->SetPulseShapeValues(newPulseValues);
    }
  }

  return pmtwf;
}

}  // namespace RAT
