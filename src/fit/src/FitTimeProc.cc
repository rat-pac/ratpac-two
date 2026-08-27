#include <TVector3.h>

#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitTimeProc.hh>
#include <RAT/Processor.hh>
#include <cmath>
#include <string>
#include <vector>

namespace RAT {

void FitTimeProc::BeginOfRun(DS::Run *run) {
  DBLinkPtr table = DB::Get()->GetLink("FIT_COMMON", "");
  fLightSpeed = table->GetD("light_speed");
  if (fLightSpeed <= 0 || fLightSpeed > 299.792458)
    throw ParamInvalid("light_speed", "light_speed in FIT_COMMON table must be > 0 and <= 299.792458 mm/ns.");

  fTransitTimeCalc = std::make_unique<RAT::TransitTimeCalculator>("g4", "rindex");
  fPMTInfo = run->GetPMTInfo();
}

void FitTimeProc::SetS(std::string param, std::string value) {
  if (param == "label") {
    if (value.empty()) throw ParamInvalid(param, "label cannot be empty.");
    fFitLabel = value;
  } else
    throw ParamUnknown(param);
}

void FitTimeProc::SetI(std::string param, int value) {
  if (param == "pmt_type") {
    fPMTtype.push_back(value);
  } else
    throw ParamUnknown(param);
}

void FitTimeProc::SetD(std::string param, double value) {
  if (param == "max_hit_time") {
    fMaxHitTime = value;
    fSetMaxHitTime = true;
  } else if (param == "min_hit_time") {
    fMinHitTime = value;
    fSetMinHitTime = true;
  } else if (param == "light_speed") {
    if (value <= 0 || value > 299.792458)
      throw ParamInvalid(param, "light_speed must be positive and <= 299.792458 mm/ns.");
    fLightSpeed = value;
  } else if (param == "wavelength") {
    if (value <= 0) throw ParamInvalid(param, "wavelength must be positive.");
    fWavelength = value;
    fSetWavelength = true;
  } else if (param == "event_position_x") {
    fPosition.SetX(value);
  } else if (param == "event_position_y") {
    fPosition.SetY(value);
  } else if (param == "event_position_z") {
    fPosition.SetZ(value);
  } else
    throw ParamUnknown(param);
}

Processor::Result FitTimeProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);

  DS::FitResult *fitT = new DS::FitResult(name, fFitLabel);
  fitT->SetEnableTime(true);

  /// Initialize FitResult parameters
  fitT->SetPosition(fPosition);
  // Figures of Merit
  fitT->SetFigureOfMerit("num_times", 0);
  fitT->SetFigureOfMerit("num_PMT", 0);

  if (inputHandler.GetNHits() <= 0) {
    ev->AddFitResult(fitT);
    return Processor::FAIL;
  }

  const TVector3 eventPos = fPosition;

  /// Loop over PMT hits and save time residuals
  int num_PMT = 0;
  std::vector<double> Times;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    // Select PMTs by type - optional
    if (fPMTtype.size() > 0) {
      int pmtType = fPMTInfo->GetType(pmtid);
      unsigned int iType = 0;
      for (iType = 0; iType < fPMTtype.size(); iType++) {
        if (fPMTtype[iType] == pmtType) break;
      }
      if (iType == fPMTtype.size()) continue;  // No match found
    }

    bool savedTime = false;
    for (double time : inputHandler.GetTimes(pmtid)) {
      // Select PMTs by time - optional
      if ((fSetMaxHitTime && time > fMaxHitTime) || (fSetMinHitTime && time < fMinHitTime)) {
        continue;
      }

      const TVector3 pmtPos = fPMTInfo->GetPosition(pmtid);

      double tt = 0;
      if (fSetWavelength) {
        TransitTimeCalculator::Result result = fTransitTimeCalc->Compute(eventPos, pmtPos, fWavelength);
        tt = result.totalTime;
      } else {
        const TVector3 hitDir = pmtPos - eventPos;
        double dist = hitDir.Mag();
        tt = dist / fLightSpeed;
      }
      double timeRes = time - tt;
      Times.push_back(timeRes);
      savedTime = true;
    }
    if (savedTime) num_PMT += 1;
  }

  unsigned int num_times = Times.size();
  if (num_times == 0) {
    ev->AddFitResult(fitT);
    return Processor::FAIL;
  }

  std::nth_element(Times.begin(), Times.begin() + num_times / 2, Times.end());
  double fitTime = Times[num_times / 2];

  /// Save results
  fitT->SetTime(fitTime);
  fitT->SetFigureOfMerit("num_times", num_times);
  fitT->SetFigureOfMerit("num_PMT", num_PMT);

  ev->AddFitResult(fitT);
  return Processor::OK;
}

}  // namespace RAT
