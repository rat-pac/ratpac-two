#include <RAT/ClassifyChargeBalance.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/PMT.hh>
#include <iostream>
#include <string>
#include <vector>

namespace RAT {

ClassifyChargeBalance::ClassifyChargeBalance() : Processor("classifychargebalance") { fLabels = {"chargebalance"}; }

Processor::Result ClassifyChargeBalance::Event(DS::Root *ds, DS::EV *ev) {
  int hitcount = ev->GetPMTCount();
  double qsumsquare = 0;
  double qsum = 0;
  for (int pmtc : ev->GetAllPMTIDs()) {
    DS::PMT *pmt = ev->GetOrCreatePMT(pmtc);
    double charge = pmt->GetCharge();
    qsumsquare += pow(charge, 2);
    qsum += charge;
  }
  double qbalance = sqrt(qsumsquare / pow(qsum, 2) - 1 / hitcount);

  DS::Classifier *classification = new DS::Classifier("ChargeBalance", fLabels);
  classification->SetClassificationResult("chargebalance", qbalance);
  ev->AddClassifierResult(classification);

  return Processor::OK;
}

}  // namespace RAT
