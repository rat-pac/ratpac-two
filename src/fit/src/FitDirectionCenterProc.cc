#include <TVector3.h>

#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitDirectionCenterProc.hh>
#include <RAT/Processor.hh>
#include <cmath>
#include <string>
#include <vector>

namespace RAT {

void FitDirectionCenterProc::BeginOfRun(DS::Run *run) {
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("Fitter", "FitDirectionCenter");
  fLightSpeed = table->GetD("light_speed");
  if (fLightSpeed <= 0 || fLightSpeed > 299.792458)
    throw ParamInvalid("light_speed", "light_speed in Fitter table must be positive and <= 299.792458 mm/ns.");

  fPMTInfo = run->GetPMTInfo();
}

void FitDirectionCenterProc::SetS(std::string param, std::string value) {
  if (param == "label") {
    if (value.empty()) throw ParamInvalid(param, "label cannot be empty.");
    fFitLabel = value;
  } else if (param == "position_fitter") {
    if (!fPosMethod.empty()) throw ParamInvalid(param, "Cannot specify both fixed and reconstructed position.");
    fPosFitter = value;
  } else if (param == "direction_fitter") {
    fDirFitter = value;
  } else
    throw ParamUnknown(param);
}

void FitDirectionCenterProc::SetI(std::string param, int value) {
  if (param == "pmt_type") {
    fPMTtype.push_back(value);
  } else
    throw ParamUnknown(param);
}

void FitDirectionCenterProc::SetD(std::string param, double value) {
  if (param == "time_resid_low") {
    if (!fCutMethod.empty() && fCutMethod != "time")
      throw ParamInvalid(param, "Cannot specify more than one cut method for time residuals.");
    fCutMethod = "time";
    fTimeResLow = value;
  } else if (param == "time_resid_up") {
    if (!fCutMethod.empty() && fCutMethod != "time")
      throw ParamInvalid(param, "Cannot specify more than one cut method for time residuals.");
    fCutMethod = "time";
    fTimeResUp = value;
  } else if (param == "time_resid_frac_low") {
    if (!fCutMethod.empty() && fCutMethod != "fraction")
      throw ParamInvalid(param, "Cannot specify more than one cut method for time residuals.");
    fCutMethod = "fraction";
    if (value < 0.0 || value >= 1.0) throw ParamInvalid(param, "time_resid_frac_low must be [0.0,1.0).");
    fTimeResFracLow = value;
  } else if (param == "time_resid_frac_up") {
    if (!fCutMethod.empty() && fCutMethod != "fraction")
      throw ParamInvalid(param, "Cannot specify more than one cut method for time residuals.");
    fCutMethod = "fraction";
    if (value <= 0.0 || value > 1.0) throw ParamInvalid(param, "time_resid_frac_up must be (0.0,1.0].");
    fTimeResFracUp = value;
  } else if (param == "light_speed") {
    if (value <= 0 || value > 299.792458)
      throw ParamInvalid(param, "light_speed must be positive and <= 299.792458 mm/ns.");
    fLightSpeed = value;
  } else if (param == "event_position_x") {
    if (!fPosFitter.empty()) throw ParamInvalid(param, "Cannot specify both fixed and reconstructed position.");
    fFixedPosition.SetX(value);
    fPosMethod = "fixed";
  } else if (param == "event_position_y") {
    if (!fPosFitter.empty()) throw ParamInvalid(param, "Cannot specify both fixed and reconstructed position.");
    fFixedPosition.SetY(value);
    fPosMethod = "fixed";
  } else if (param == "event_position_z") {
    if (!fPosFitter.empty()) throw ParamInvalid(param, "Cannot specify both fixed and reconstructed position.");
    fFixedPosition.SetZ(value);
    fPosMethod = "fixed";
  } else if (param == "event_time") {
    fFixedTime = value;
  } else if (param == "event_drive") {
    fDrive = value;
  } else
    throw ParamUnknown(param);
}

Processor::Result FitDirectionCenterProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);

  DS::FitResult *fitDC = new DS::FitResult(name, fFitLabel);

  /// Initialize ALL parameters with placeholder values
  fitDC->SetEnablePosition(false);
  fitDC->SetValidPosition(false);
  fitDC->SetPosition(TVector3(0, 0, 0));
  fitDC->SetEnableDirection(false);
  fitDC->SetValidDirection(false);
  fitDC->SetDirection(TVector3(0, 0, 0));
  // Figures of Merit
  fitDC->SetFigureOfMerit("num_PMT", 0);
  fitDC->SetFigureOfMerit("time_resid_low", NAN);
  fitDC->SetFigureOfMerit("time_resid_up", NAN);
  fitDC->SetFigureOfMerit("angle_mean", NAN);
  fitDC->SetFigureOfMerit("angle_stddev", NAN);
  fitDC->SetFigureOfMerit("angle_octile7", NAN);
  fitDC->SetFigureOfMerit("angle_quartile3", NAN);
  fitDC->SetFigureOfMerit("angle_median", NAN);
  fitDC->SetFigureOfMerit("angle_quartile1", NAN);
  fitDC->SetFigureOfMerit("angle_octile1", NAN);

  int numPMTs = inputHandler.GetNHits();
  if (numPMTs <= 0) {
    ev->AddFitResult(fitDC);
    return Processor::FAIL;
  }

  /// Set event position

  // Initialize at fixed values
  TVector3 eventPos = fFixedPosition;
  bool validPos = true;
  double eventTime = fFixedTime;

  // Get reconstructed position from a fit result
  if (fPosMethod != "fixed") {
    std::vector<RAT::DS::FitResult *> fits = ev->GetFitResults();
    if (fits.size() == 0) {
      Log::Die("FitDirectionCenterProc: No position specified (fixed or reconstructed).");
    }

    RAT::DS::FitResult *fit;
    if (fPosFitter.empty()) {       // If fitter not specified
      fit = fits[fits.size() - 1];  // use last fit result
    } else {
      bool foundPosFitter = false;
      for (unsigned int iFit = 0; iFit < fits.size(); iFit++) {
        if (fPosFitter == (fits[iFit])->GetFullName()) {
          fit = fits[iFit];
          foundPosFitter = true;
          break;
        }
      }
      if (!foundPosFitter) {
        Log::Die("FitDirectionCenterProc: Position fitter \'" + fPosFitter + "\' not found.  Check name.");
      }
    }

    if (fit->GetEnablePosition()) {
      eventPos = fit->GetPosition();
      validPos = fit->GetValidPosition();
    } else {
      ev->AddFitResult(fitDC);
      return Processor::FAIL;
    }
    if (fit->GetEnableTime()) {
      eventTime = fit->GetTime();
    }
  }

  // Apply drive correction
  bool applyDrive = (!fDirFitter.empty() && fDrive != 0.0);
  if (applyDrive) {
    std::vector<RAT::DS::FitResult *> fits = ev->GetFitResults();
    if (fits.size() == 0) {
      Log::Die("FitDirectionCenterProc: No reconstructed direction available for drive correction.");
    }

    RAT::DS::FitResult *fit;
    bool foundDirFitter = false;
    for (unsigned int iFit = 0; iFit < fits.size(); iFit++) {
      if (fDirFitter == (fits[iFit])->GetFullName()) {
        fit = fits[iFit];
        foundDirFitter = true;
        break;
      }
    }
    if (!foundDirFitter) {
      Log::Die("FitDirectionCenterProc: Direction fitter \'" + fDirFitter + "\' not found.  Check that it was run.");
    }

    TVector3 eventDir;
    bool validDir = true;
    if (fit->GetEnableDirection()) {
      eventDir = fit->GetDirection();
      validDir = fit->GetValidDirection();
    } else {
      ev->AddFitResult(fitDC);
      return Processor::FAIL;
    }
    // Save drive correction
    eventPos -= fDrive * eventDir;
    fitDC->SetEnablePosition(true);
    if (validPos && validDir) fitDC->SetValidPosition(true);
    fitDC->SetPosition(eventPos);

  } else if (fDrive != 0.0 && fDirFitter.empty()) {
    Log::Die("FitDirectionCenterProc: No direction fitter specified while drive value is specified.");
  } else if (fDrive == 0.0 && !fDirFitter.empty()) {
    Log::Die("FitDirectionCenterProc: No drive value specified while direction fitter \'" + fPosFitter +
             "\' is specified.");
  }

  /// If fractional time cuts specified, determine cut times
  double timeResLowFrac = -9999, timeResUpFrac = 9999;
  if (fCutMethod == "fraction") {
    std::vector<double> pmtTimes;
    pmtTimes.reserve(numPMTs);
    for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
      // Select PMTs by type
      if (fPMTtype.size() > 0) {
        int pmtType = fPMTInfo->GetType(pmtid);
        unsigned int iType = 0;
        for (iType = 0; iType < fPMTtype.size(); iType++) {
          if (fPMTtype[iType] == pmtType) break;
        }
        if (iType == fPMTtype.size()) continue;  // No match found
      }

      TVector3 pmtPos = fPMTInfo->GetPosition(pmtid);
      TVector3 hitDir = pmtPos - eventPos;

      double transitTime = hitDir.Mag() / fLightSpeed;
      double timeResidual = inputHandler.GetTime(pmtid) - transitTime - eventTime;
      pmtTimes.push_back(timeResidual);
    }
    if (pmtTimes.empty()) {  // No PMTs selected
      ev->AddFitResult(fitDC);
      return Processor::FAIL;
    }
    int up = round(fTimeResFracUp * (pmtTimes.size() - 1));
    std::nth_element(pmtTimes.begin(), pmtTimes.begin() + up, pmtTimes.end());
    timeResUpFrac = pmtTimes.at(up);
    int low = round(fTimeResFracLow * (pmtTimes.size() - 1));
    std::nth_element(pmtTimes.begin(), pmtTimes.begin() + low, pmtTimes.end());
    timeResLowFrac = pmtTimes.at(low);
  }

  /// Apply time cuts and determine photon path directions
  TVector3 directionSum(0.0, 0.0, 0.0);
  double timeResLow = 9999, timeResUp = -9999;
  int numDir = 0;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    // Select PMTs by type
    if (fPMTtype.size() > 0) {
      int pmtType = fPMTInfo->GetType(pmtid);
      unsigned int iType = 0;
      for (iType = 0; iType < fPMTtype.size(); iType++) {
        if (fPMTtype[iType] == pmtType) break;
      }
      if (iType == fPMTtype.size()) continue;  // No match found
    }

    TVector3 pmtPos = fPMTInfo->GetPosition(pmtid);
    TVector3 hitDir = pmtPos - eventPos;

    // Apply time residual cuts
    if (!fCutMethod.empty()) {
      double transitTime = hitDir.Mag() / fLightSpeed;
      double timeResidual = inputHandler.GetTime(pmtid) - transitTime - eventTime;
      if ((fCutMethod == "time" && (timeResidual < fTimeResLow || timeResidual > fTimeResUp)) ||
          (fCutMethod == "fraction" && (timeResidual < timeResLowFrac || timeResidual > timeResUpFrac)))
        continue;
      if (timeResidual < timeResLow)
        timeResLow = timeResidual;
      else if (timeResidual > timeResUp)
        timeResUp = timeResidual;
    }

    directionSum += hitDir.Unit();
    numDir += 1;
  }
  if (numDir == 0) {
    ev->AddFitResult(fitDC);
    return Processor::FAIL;
  }
  TVector3 directionMean = directionSum.Unit();

  /// Calculate mean angle between reconstructed direction and photon path directions
  double angleSum = 0, angle = 0;
  std::vector<double> angles;
  numDir = 0;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    // Select PMTs by type
    if (fPMTtype.size() > 0) {
      int pmtType = fPMTInfo->GetType(pmtid);
      unsigned int iType = 0;
      for (iType = 0; iType < fPMTtype.size(); iType++) {
        if (fPMTtype[iType] == pmtType) break;
      }
      if (iType == fPMTtype.size()) continue;  // No match found
    }

    TVector3 pmtPos = fPMTInfo->GetPosition(pmtid);
    TVector3 hitDir = pmtPos - eventPos;

    // Apply time residual cuts
    if (!fCutMethod.empty()) {
      double transitTime = hitDir.Mag() / fLightSpeed;
      double timeResidual = inputHandler.GetTime(pmtid) - transitTime - eventTime;
      if ((fCutMethod == "time" && (timeResidual < fTimeResLow || timeResidual > fTimeResUp)) ||
          (fCutMethod == "fraction" && (timeResidual < timeResLowFrac || timeResidual > timeResUpFrac)))
        continue;
    }

    angle = acos(directionMean.Dot(hitDir.Unit()));
    angles.push_back(angle);
    angleSum += angle;
    numDir += 1;
  }

  double angleMean = 400, angleO7 = 400, angleQ3 = 400, angleMedian = 400, angleQ1 = 400, angleO1 = 400;
  if (numDir > 0) {
    angleMean = angleSum / numDir;
    int numAngles = angles.size();
    /// ======= Percentiles ======
    /// ---- Octile 7
    int m = numAngles * 7 / 8 - 1;
    if (m < 0) m = 0;
    std::nth_element(angles.begin(), angles.begin() + m, angles.end());
    angleO7 = angles[m];
    // if percentile sits between two values, average them
    if (std::fmod(numAngles * 7.0 / 8.0, 1.0) == 0.0) {
      m += 1;
      if (m >= numAngles) m = numAngles - 1;
      std::nth_element(angles.begin(), angles.begin() + m, angles.end());
      angleO7 = (angleO7 + angles[m]) / 2.0;
    }
    /// ---- Quartile 3
    m = numAngles * 3 / 4 - 1;
    if (m < 0) m = 0;
    std::nth_element(angles.begin(), angles.begin() + m, angles.end());
    angleQ3 = angles[m];
    // if percentile sits between two values, average them
    if (std::fmod(numAngles * 3.0 / 4.0, 1.0) == 0.0) {
      m += 1;
      if (m >= numAngles) m = numAngles - 1;
      std::nth_element(angles.begin(), angles.begin() + m, angles.end());
      angleQ3 = (angleQ3 + angles[m]) / 2.0;
    }
    /// ---- Median
    m = numAngles / 2 - 1;
    if (m < 0) m = 0;
    std::nth_element(angles.begin(), angles.begin() + m, angles.end());
    angleMedian = angles[m];
    // if percentile sits between two values, average them
    if (std::fmod(numAngles / 2.0, 1.0) == 0.0) {
      m += 1;
      if (m >= numAngles) m = numAngles - 1;
      std::nth_element(angles.begin(), angles.begin() + m, angles.end());
      angleMedian = (angleMedian + angles[m]) / 2.0;
    }
    /// ---- Quartile 1
    m = numAngles / 4 - 1;
    if (m < 0) m = 0;
    std::nth_element(angles.begin(), angles.begin() + m, angles.end());
    angleQ1 = angles[m];
    // if percentile sits between two values, average them
    if (std::fmod(numAngles / 4.0, 1.0) == 0.0) {
      m += 1;
      if (m >= numAngles) m = numAngles - 1;
      std::nth_element(angles.begin(), angles.begin() + m, angles.end());
      angleQ1 = (angleQ1 + angles[m]) / 2.0;
    }
    /// ---- Octile 1
    m = numAngles / 8 - 1;
    if (m < 0) m = 0;
    std::nth_element(angles.begin(), angles.begin() + m, angles.end());
    angleO1 = angles[m];
    // if percentile sits between two values, average them
    if (std::fmod(numAngles / 8.0, 1.0) == 0.0) {
      m += 1;
      if (m >= numAngles) m = numAngles - 1;
      std::nth_element(angles.begin(), angles.begin() + m, angles.end());
      angleO1 = (angleO1 + angles[m]) / 2.0;
    }
  }

  /// Calculate deviation about mean angle
  double angleSum2 = 0;
  double angleDevMean = 400;
  angle = 0;
  numDir = 0;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    // Select PMTs by type
    if (fPMTtype.size() > 0) {
      int pmtType = fPMTInfo->GetType(pmtid);
      unsigned int iType = 0;
      for (iType = 0; iType < fPMTtype.size(); iType++) {
        if (fPMTtype[iType] == pmtType) break;
      }
      if (iType == fPMTtype.size()) continue;  // No match found
    }

    TVector3 pmtPos = fPMTInfo->GetPosition(pmtid);
    TVector3 hitDir = pmtPos - eventPos;

    // Apply time residual cuts
    if (!fCutMethod.empty()) {
      double transitTime = hitDir.Mag() / fLightSpeed;
      double timeResidual = inputHandler.GetTime(pmtid) - transitTime - eventTime;
      if ((fCutMethod == "time" && (timeResidual < fTimeResLow || timeResidual > fTimeResUp)) ||
          (fCutMethod == "fraction" && (timeResidual < timeResLowFrac || timeResidual > timeResUpFrac)))
        continue;
    }

    angle = acos(directionMean.Dot(hitDir.Unit()));
    angleSum2 += (angle - angleMean) * (angle - angleMean);
    numDir += 1;
  }
  if (numDir > 1) {
    angleDevMean = sqrt(angleSum2 / (numDir - 1));
  }

  /// Save results
  if (directionMean.Mag2() > 0) {
    fitDC->SetEnableDirection(true);
    fitDC->SetValidDirection(true);
    fitDC->SetDirection(directionMean.Unit());
    fitDC->SetFigureOfMerit("num_PMT", numDir);
    fitDC->SetFigureOfMerit("time_resid_low", timeResLow);
    fitDC->SetFigureOfMerit("time_resid_up", timeResUp);
    fitDC->SetFigureOfMerit("angle_mean", angleMean);
    fitDC->SetFigureOfMerit("angle_stddev", angleDevMean);
    fitDC->SetFigureOfMerit("angle_octile7", angleO7);
    fitDC->SetFigureOfMerit("angle_quartile3", angleQ3);
    fitDC->SetFigureOfMerit("angle_median", angleMedian);
    fitDC->SetFigureOfMerit("angle_quartile1", angleQ1);
    fitDC->SetFigureOfMerit("angle_octile1", angleO1);
  }

  ev->AddFitResult(fitDC);
  return Processor::OK;
}

}  // namespace RAT
