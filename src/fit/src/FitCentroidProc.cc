#include <TVector3.h>

#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitCentroidProc.hh>
#include <RAT/Processor.hh>
#include <cmath>
#include <string>

namespace RAT {
void FitCentroidProc::SetD(std::string param, double value) {
  if (param == "power") {
    fPower = value;
  } else if (param == "rescale") {
    fRescale = value;
  } else {
    throw ParamUnknown(param);
  }
}

Processor::Result FitCentroidProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);
  double totalQ = 0;
  TVector3 centroid(0.0, 0.0, 0.0);

  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    double Qpow = 0.0;
    Qpow = pow(inputHandler.GetCharge(pmtid), fPower);
    totalQ += Qpow;

    DS::Run *run = DS::RunStore::Get()->GetRun(ds);
    DS::PMTInfo *pmtinfo = run->GetPMTInfo();
    TVector3 pmtpos = pmtinfo->GetPosition(pmtid);

    if (fRescale != 1.0) {
      pmtpos.SetMag(pmtpos.Mag() * fRescale);
    }

    centroid += Qpow * pmtpos;
  }

  DS::FitResult *fit = new DS::FitResult("FitCentroid");
  fit->SetEnablePosition(true);
  if (totalQ) {
    centroid *= 1.0 / totalQ;
    fit->SetPosition(centroid);
  } else {
    fit->SetValidPosition(false);
  }
  ev->AddFitResult(fit);

  return Processor::OK;
}

}  // namespace RAT
