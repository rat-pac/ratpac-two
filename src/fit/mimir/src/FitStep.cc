#include <mimir/Factory.hh>
#include <mimir/FitStep.hh>

namespace RAT::Mimir {

bool FitStep::Configure(RAT::DBLinkPtr db_link) {
  std::string optimizer_name = db_link->GetS("optimizer_name");
  std::string optimizer_index = db_link->GetS("optimizer_config");
  optimizer = Factory<Optimizer>::GetInstance().make_and_configure(optimizer_name, optimizer_index);
  std::string cost_name = db_link->GetS("cost_name");
  std::string cost_index = db_link->GetS("cost_config");
  cost_function = Factory<Cost>::GetInstance().make_and_configure(cost_name, cost_index);
  set_status(db_link, "position_time_status", position_time_status);
  set_status(db_link, "direction_status", direction_status);
  set_status(db_link, "energy_status", energy_status);
  return true;
}

void FitStep::set_status(RAT::DBLinkPtr db_link, const std::string& field_name,
                         std::vector<ParamStatus>& status_vector) {
  try {
    int common_status = db_link->GetI(field_name);
    for (size_t i = 0; i < status_vector.size(); ++i) {
      status_vector[i] = static_cast<ParamStatus>(common_status);
    }
  } catch (const RAT::DBWrongTypeError& e) {
    std::vector<int> status = db_link->GetIArray(field_name);
    if (status.size() != status_vector.size()) {
      RAT::Log::Die("provided status vector size does not match expected size");
    }
    for (size_t i = 0; i < status_vector.size(); ++i) {
      status_vector[i] = static_cast<ParamStatus>(status[i]);
    }
  } catch (const RAT::DBNotFoundError& e) {
    status_vector.assign(status_vector.size(), ParamStatus::INACTIVE);  // Default to inactive if not found
  }
}

void FitStep::Execute(ParamSet& params) {
  cost_function->ClearHits();
  std::vector<int> pmtids = input_handler->GetAllHitPMTIDs();
  for (int pmtid : pmtids) {
    cost_function->AddHit(pmtid, input_handler->GetTime(pmtid), input_handler->GetCharge(pmtid));
  }
  params.position_time.set_status(position_time_status);
  params.direction.set_status(direction_status);
  params.energy.set_status(energy_status);
  optimizer->Minimize(cost_function.get(), params);
}

}  // namespace RAT::Mimir
