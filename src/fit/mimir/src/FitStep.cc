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
  position_time_status.resize(4, ParamStatus::INACTIVE);
  direction_status.resize(2, ParamStatus::INACTIVE);
  energy_status.resize(1, ParamStatus::INACTIVE);
  set_status(db_link, "position_time_status", position_time_status);
  set_status(db_link, "direction_status", direction_status);
  set_status(db_link, "energy_status", energy_status);

  position_time_lb.resize(4, std::numeric_limits<double>::lowest());
  position_time_ub.resize(4, std::numeric_limits<double>::max());
  direction_lb = {0, -CLHEP::pi};
  direction_ub = {CLHEP::pi, CLHEP::pi};
  energy_lb = {0.0};
  energy_ub = {std::numeric_limits<double>::max()};
  set_bounds(db_link, {"x", "y", "z", "t"}, position_time_lb, position_time_ub);
  set_bounds(db_link, {"theta", "phi"}, direction_lb, direction_ub);
  set_bounds(db_link, {"energy"}, energy_lb, energy_ub);
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

void FitStep::set_bounds(RAT::DBLinkPtr db_link, const std::vector<std::string>& names, std::vector<double>& lb,
                         std::vector<double>& ub) {
  if (names.size() != lb.size() || names.size() != ub.size()) {
    RAT::Log::Die("Mismatch in number of bounds provided.");
  }
  for (size_t i = 0; i < names.size(); ++i) {
    try {
      std::vector<double> bounds = db_link->GetDArray(names[i] + "_bound");
      if (bounds.size() != 2) {
        RAT::Log::Die("Expected 2 bounds for " + names[i] + "_bound, but got " + std::to_string(bounds.size()));
      }
      lb[i] = bounds[0];
      ub[i] = bounds[1];
    } catch (const RAT::DBNotFoundError& e) {
    }
  }
}

void FitStep::Execute(ParamSet& params) {
  cost_function->ClearHits();
  std::vector<int> pmtids = input_handler->GetAllHitPMTIDs();
  for (int pmtid : pmtids) {
    cost_function->AddHit(pmtid, input_handler->GetTime(pmtid), input_handler->GetCharge(pmtid));
  }
  params.position_time.set_status(position_time_status);
  params.position_time.set_lower_bounds(position_time_lb);
  params.position_time.set_upper_bounds(position_time_ub);
  params.direction.set_status(direction_status);
  params.direction.set_lower_bounds(direction_lb);
  params.direction.set_upper_bounds(direction_ub);
  params.energy.set_status(energy_status);
  params.energy.set_lower_bounds(energy_lb);
  params.energy.set_upper_bounds(energy_ub);
  optimizer->Minimize(cost_function.get(), params);
}

}  // namespace RAT::Mimir
