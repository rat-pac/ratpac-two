#include <RAT/ClassifyChargeBalance.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/PMT.hh>
#include <iostream>
#include <string>
#include <vector>

namespace RAT {

Processor::Result ClassifyChargeBalance::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);

  DS::Classifier *classification = new DS::Classifier("ChargeBalance", fLabels);

  // Initialize ALL parameters with placeholder values
  classification->SetClassificationResult("chargebalance", NAN);

  double hitcount = static_cast<double>(inputHandler.GetNHits());
  if (hitcount <= 1) {
    ev->AddClassifierResult(classification);
    return Processor::FAIL;
  }

  double qsumsquare = 0;
  double qsum = 0;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    double charge = inputHandler.GetCharge(pmtid);
    qsumsquare += pow(charge, 2);
    qsum += charge;
  }
  if (qsum == 0.0) {
    ev->AddClassifierResult(classification);
    return Processor::FAIL;
  }

  double qbalance = hitcount * sqrt(1.0 / (hitcount - 1.0) * (qsumsquare / pow(qsum, 2) - 1.0 / hitcount));

  classification->SetClassificationResult("chargebalance", qbalance);

  ev->AddClassifierResult(classification);
  return Processor::OK;
}

}  // namespace RAT
