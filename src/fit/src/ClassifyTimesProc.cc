#include <TVector3.h>

#include <RAT/ClassifyTimesProc.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/Processor.hh>
#include <cmath>
#include <string>
#include <vector>

namespace RAT {

void ClassifyTimesProc::BeginOfRun(DS::Run *run) {
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("Classifier", "ClassifyTimes");

  fLightSpeed = table->GetD("light_speed");
  if (fLightSpeed <= 0 || fLightSpeed > 299.792458)
    throw ParamInvalid("light_speed", "light_speed in Classifier table must be > 0 and <= 299.792458 mm/ns.");

  fNumerTimeResLow = table->GetD("numer_time_resid_low");
  fNumerTimeResUp  = table->GetD("numer_time_resid_up");
  if (fNumerTimeResLow > fNumerTimeResUp)
    throw ParamInvalid("numer_time_resid_low", "numer_time_resid_low in Classifier table must be <= numer_time_resid_up.");

  fPMTInfo = run->GetPMTInfo();
}

void ClassifyTimesProc::SetS(std::string param, std::string value) {
  if (param == "classifier_name") {
    if (value.empty()) throw ParamInvalid(param, "classifier_name cannot be empty.");
    fClassifierName = value;
  } else if (param == "position_fitter") {
    if (!fPosMethod.empty()) throw ParamInvalid(param, "Cannot specify both fixed and reconstructed position.");
    fPosFitter = value;
  } else
    throw ParamUnknown(param);
}

void ClassifyTimesProc::SetI(std::string param, int value) {
  if (param == "pmt_type") {
    fPMTtype.push_back(value);
  } else if (param == "verbose") {
    fVerbose = value;
  } else
    throw ParamUnknown(param);
}

void ClassifyTimesProc::SetD(std::string param, double value) {
  /// Time boundary parameters for central moments
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
    /// Time boundaries for ratio
  } else if (param == "numer_time_resid_low") {
    fNumerTimeResLow = value;
  } else if (param == "numer_time_resid_up") {
    fNumerTimeResUp = value;
  } else if (param == "denom_time_resid_low") {
    fDenomTimeResLow = value;
  } else if (param == "denom_time_resid_up") {
    fDenomTimeResUp = value;
    /// Event position and time input
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
  } else
    throw ParamUnknown(param);
}

Processor::Result ClassifyTimesProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);

  if (fVerbose >= 1) {
    fParamNames.push_back("num_PMT");
    fParamNames.push_back("num_PMT_numer");
    fParamNames.push_back("num_PMT_denom");
  }
  if (fVerbose >= 2) {
    fParamNames.push_back("time_resid_low");
    fParamNames.push_back("time_resid_up");
  }

  DS::Classifier *clf = new DS::Classifier(fClassifierName, fParamNames);

  /// Initialize ALL parameters with placeholder values
  clf->SetClassificationResult("ratio", NAN);
  clf->SetClassificationResult("mean", NAN);
  clf->SetClassificationResult("stddev", NAN);
  clf->SetClassificationResult("skewness", NAN);
  clf->SetClassificationResult("kurtosis", NAN);

  if (fVerbose >= 1) {
    clf->SetClassificationResult("num_PMT", 0.0);
    clf->SetClassificationResult("num_PMT_numer", 0.0);
    clf->SetClassificationResult("num_PMT_denom", 0.0);
  }
  if (fVerbose >= 2) {
    clf->SetClassificationResult("time_resid_low", NAN);
    clf->SetClassificationResult("time_resid_up", NAN);
  }

  int numPMTs = inputHandler.GetNHits();
  if (numPMTs <= 0) {
    ev->AddClassifierResult(clf);
    return Processor::FAIL;
  }

  /// Get event position

  // Initialize at fixed values
  TVector3 eventPos = fFixedPosition;
  double eventTime = fFixedTime;

  // Get reconstructed position from a fit result
  if (fPosMethod != "fixed") {
    std::vector<RAT::DS::FitResult *> fits = ev->GetFitResults();
    if (fits.size() == 0) {
      Log::Die("ClassifyTimesProc: No position available (fixed or reconstructed).");
    }

    RAT::DS::FitResult *fit;
    if (fPosFitter.empty()) {       // If fitter not specified
      fit = fits[fits.size() - 1];  // use last fit result
    } else {
      fit = inputHandler.FindFitResult(fPosFitter);
      if (fit == nullptr)
        Log::Die("ClassifyTimesProc: Position fitter \'" + fPosFitter + "\' not found.  Check name.");
    }

    if (fit->GetEnablePosition()) {
      eventPos = fit->GetPosition();
    } else {
      ev->AddClassifierResult(clf);
      return Processor::FAIL;
    }
    if (fit->GetEnableTime()) {
      eventTime = fit->GetTime();
    }
  }

  /// If fractional cuts specified for central moments, determine cut times
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
    if (pmtTimes.empty()) {  // No PMTs selected - Force a fail later in time residual cuts
      timeResUpFrac = -9998.9999;
      timeResLowFrac = -9999;
    } else {
      int up = round(fTimeResFracUp * (pmtTimes.size() - 1));
      std::nth_element(pmtTimes.begin(), pmtTimes.begin() + up, pmtTimes.end());
      timeResUpFrac = pmtTimes.at(up);
      int low = round(fTimeResFracLow * (pmtTimes.size() - 1));
      std::nth_element(pmtTimes.begin(), pmtTimes.begin() + low, pmtTimes.end());
      timeResLowFrac = pmtTimes.at(low);
    }
  }

  /// Apply time cuts and determine PMT counts and first central moment of time residuals
  int numPMT = 0, numPMTnumer = 0, numPMTdenom = 0;
  double timeResLow = 9999, timeResUp = -9999, sumTimes = 0;
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

    // Apply time residual cuts for ratio
    if (timeResidual <= fNumerTimeResUp && timeResidual >= fNumerTimeResLow)
      numPMTnumer += 1;
    if ((timeResidual <= fDenomTimeResUp && timeResidual >= fDenomTimeResLow) ||
        fDenomTimeResUp == fDenomTimeResLow)  // Use full range if cuts are equal
      numPMTdenom += 1;

    // Apply time residual cuts for central moments
    if (!fCutMethod.empty()) {
      if ((fCutMethod == "time" &&
           ((timeResidual < fTimeResLow || timeResidual > fTimeResUp) || fTimeResLow == fTimeResUp)) ||
          (fCutMethod == "fraction" &&
           ((timeResidual < timeResLowFrac || timeResidual > timeResUpFrac) || timeResLowFrac == timeResUpFrac)))
        continue;
    }
    numPMT += 1;
    sumTimes += timeResidual;

    if (timeResidual < timeResLow)
      timeResLow = timeResidual;
    else if (timeResidual > timeResUp)
      timeResUp = timeResidual;
  }
  double mean = sumTimes / numPMT;

  /// Apply time cuts and determine second, third, and fourth central moments of time residuals
  int num = 0;
  double sumTimes2 = 0, sumTimes3 = 0, sumTimes4 = 0, diff = 0;
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

    // Apply time residual cuts for central moments
    if (!fCutMethod.empty()) {
      if ((fCutMethod == "time" &&
           ((timeResidual < fTimeResLow || timeResidual > fTimeResUp) || fTimeResLow == fTimeResUp)) ||
          (fCutMethod == "fraction" &&
           ((timeResidual < timeResLowFrac || timeResidual > timeResUpFrac) || timeResLowFrac == timeResUpFrac)))
        continue;
    }
    num += 1;
    diff = timeResidual - mean;
    sumTimes2 += pow(diff, 2);
    sumTimes3 += pow(diff, 3);
    sumTimes4 += pow(diff, 4);
  }
  double sigma=0, skew=0, kurt=0;
  if (num > 1) {
    sigma = sqrt(sumTimes2 / (num - 1));                               // unbiased standard deviation
    if (num > 2) {
      skew = sumTimes3 / pow(sigma, 3) * num / (num - 1) / (num - 2);  // standardized unbiased skewness
      if (num > 3)
        kurt = sumTimes4 / pow(sigma, 4) * num * (num + 1) / (num - 1) / (num - 2) / (num - 3) -
               3.0 * (num - 1) * (num - 1) / (num - 2) / (num - 3);    // standardized unbiased excess kurtosis
    }
  }

  /// Save results
  if (numPMTdenom > 0)
    clf->SetClassificationResult("ratio", static_cast<double>(numPMTnumer) / numPMTdenom);

  if (numPMT > 0) {
    clf->SetClassificationResult("mean", mean);
    if (num > 1) {
      clf->SetClassificationResult("stddev", sigma);
      if (num > 2) {
        clf->SetClassificationResult("skewness", skew);
	if (num > 3)
          clf->SetClassificationResult("kurtosis", kurt);
      }
    }
  }

  if (fVerbose >= 1) {
    clf->SetClassificationResult("num_PMT", static_cast<double>(numPMT));
    clf->SetClassificationResult("num_PMT_numer", static_cast<double>(numPMTnumer));
    clf->SetClassificationResult("num_PMT_denom", static_cast<double>(numPMTdenom));
  }
  if (fVerbose >= 2) {
    clf->SetClassificationResult("time_resid_low", timeResLow);
    clf->SetClassificationResult("time_resid_up", timeResUp);
  }

  ev->AddClassifierResult(clf);
  return Processor::OK;
}

}  // namespace RAT
