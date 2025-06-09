#include <mimir/PositionTime_PMTTypeTimeResidual.hh>

namespace RAT::Mimir {

bool PositionTime_PMTTypeTimeResidual::Configure(RAT::DBLinkPtr db_link) {
  if (!optimizer.Configure(db_link)) {
    RAT::Log::Die("PositionTime_PMTTypeTimeResidual: Failed to configure optimizer.");
  }
  RAT::DBLinkPtr lCost = RAT::DB::Get()->GetLink("MIMIR_PMTTypeTimeResidual", "");
  if (!cost_function.Configure(lCost)) {
    RAT::Log::Die("PositionTime_PMTTypeTimeResidual: Failed to configure cost function.");
  }
  return true;
}

void PositionTime_PMTTypeTimeResidual::Execute(ParamSet &params) {
  cost_function.ClearHits();
  std::vector<int> pmtids = input_handler->GetAllHitPMTIDs();
  for (int pmtid : pmtids) {
    cost_function.AddHit(pmtid, input_handler->GetTime(pmtid));
  }
  params.position_time.set_all_status(ParamStatus::ACTIVE);
  optimizer.Minimize(cost_function, params);
}

}  // namespace RAT::Mimir
