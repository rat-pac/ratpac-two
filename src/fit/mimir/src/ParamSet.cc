#include <RAT/Log.hh>
#include <mimir/ParamSet.hh>
#include <sstream>

namespace Mimir {

std::vector<double> ParamField::active_values() const {
  std::vector<double> values;
  for (const auto& component : components) {
    if (component.is_active()) {
      values.push_back(component.value);
    }
  }
  return values;
}

std::vector<double> ParamField::used_values() const {
  std::vector<double> values;
  for (const auto& component : components) {
    if (component.is_used()) {
      values.push_back(component.value);
    }
  }
  return values;
}

std::vector<double> ParamField::active_lower_bounds() const {
  std::vector<double> bounds;
  for (const auto& component : components) {
    if (component.is_active()) {
      bounds.push_back(component.lower_bound);
    }
  }
  return bounds;
}

std::vector<double> ParamField::active_upper_bounds() const {
  std::vector<double> bounds;
  for (const auto& component : components) {
    if (component.is_active()) {
      bounds.push_back(component.upper_bound);
    }
  }
  return bounds;
}

void ParamField::set_all_lower_bounds(double value) {
  for (auto& component : components) {
    component.lower_bound = value;
  }
}

void ParamField::set_all_upper_bounds(double value) {
  for (auto& component : components) {
    component.upper_bound = value;
  }
}

void ParamField::set_all_fit_valid(bool valid) {
  for (auto& component : components) {
    component.fit_valid = valid;
  }
}

void ParamField::set_all_status(ParamStatus status) {
  for (auto& component : components) {
    component.status = status;
  }
}

void ParamField::set_status(std::vector<ParamStatus> status_vector) {
  if (status_vector.size() != components.size()) {
    std::stringstream msg;
    msg << "Mismatch in number of status codes provided. Expected " << components.size() << ", but got "
        << status_vector.size();
    RAT::Log::Die(msg.str());
  }
  for (size_t i = 0; i < components.size() && i < status_vector.size(); ++i) {
    components[i].status = status_vector[i];
  }
}

void ParamField::set_values(std::vector<double> values) { set_values(values.data(), values.size()); }

void ParamField::set_values(const double* values, size_t n) {
  if (n != components.size()) {
    std::stringstream msg;
    msg << "Mismatch in number of values provided. Expected " << components.size() << ", but got " << n;
    RAT::Log::Die(msg.str());
  }
  for (size_t i = 0; i < components.size() && i < n; ++i) {
    components[i].value = values[i];
  }
}

void ParamField::set_lower_bounds(std::vector<double> lower_bounds) {
  set_lower_bounds(lower_bounds.data(), lower_bounds.size());
}

void ParamField::set_lower_bounds(const double* lower_bounds, size_t n) {
  if (n != components.size()) {
    std::stringstream msg;
    msg << "Mismatch in number of bounds provided. Expected " << components.size() << ", but got " << n;
    RAT::Log::Die(msg.str());
  }
  for (size_t i = 0; i < components.size() && i < n; ++i) {
    components[i].lower_bound = lower_bounds[i];
  }
}

void ParamField::set_upper_bounds(std::vector<double> upper_bounds) {
  set_upper_bounds(upper_bounds.data(), upper_bounds.size());
}
void ParamField::set_upper_bounds(const double* upper_bounds, size_t n) {
  if (n != components.size()) {
    std::stringstream msg;
    msg << "Mismatch in number of bounds provided. Expected " << components.size() << ", but got " << n;
    RAT::Log::Die(msg.str());
  }
  for (size_t i = 0; i < components.size() && i < n; ++i) {
    components[i].upper_bound = upper_bounds[i];
  }
}

std::vector<double> ParamSet::to_active_vector() const {
  std::vector<double> values;
  for (const ParamField& field : {position_time, direction, energy}) {
    auto active_vals = field.active_values();
    values.insert(values.end(), active_vals.begin(), active_vals.end());
  }
  return values;
}

std::vector<ParamComponent> ParamSet::to_active_components() const {
  std::vector<ParamComponent> active_components;
  active_components.clear();
  for (const ParamField& field : {position_time, direction, energy}) {
    for (const ParamComponent& component : field.components) {
      if (component.is_active()) {
        active_components.push_back(component);
      }
    }
  }
  return active_components;
}

void ParamSet::update_active(const std::vector<double>& values) { return update_active(values.data(), values.size()); }

void ParamSet::update_active(const double* values, size_t n) {
  size_t index = 0;
  for (ParamField* field : {&position_time, &direction, &energy}) {
    if (index > n) {
      std::stringstream msg;
      msg << "Not enough values provided to from_fit_vector. Expected at least " << index << ", but got " << n;
      RAT::Log::Die(msg.str());
    }
    for (size_t idx_in_field = 0; idx_in_field < field->components.size(); ++idx_in_field) {
      if (field->components[idx_in_field].is_active()) {
        field->components[idx_in_field].value = values[index++];
      }
    }
  }
  if (index != n) {
    std::stringstream msg;
    msg << "Too many values provided to from_fit_vector. Expected " << index << ", but got " << n;
    RAT::Log::Die(msg.str());
  }
}

void ParamSet::update_active(const double* values, size_t n) {
  size_t index = 0;
  for (ParamField* field : {&position_time, &direction, &energy}) {
    if (index > n) {
      std::stringstream msg;
      msg << "Not enough values provided to from_fit_vector. Expected at least " << index << ", but got " << n;
      RAT::Log::Die(msg.str());
    }
    for (size_t idx_in_field = 0; idx_in_field < field->components.size(); ++idx_in_field) {
      if (field->components[idx_in_field].is_active()) {
        field->components[idx_in_field].value = values[index++];
      }
    }
  }
  if (index != n) {
    std::stringstream msg;
    msg << "Too many values provided to from_fit_vector. Expected " << index << ", but got " << n;
    RAT::Log::Die(msg.str());
  }
}

ParamSet ParamSet::from_active_vector(const std::vector<double>& values) const {
  ParamSet result = *this;
  result.update_active(values);
  return result;
}

void ParamSet::set_active_fit_valid(bool valid) {
  for (ParamField* field : {&position_time, &direction, &energy}) {
    for (ParamComponent& component : field->components) {
      if (component.is_active()) {
        component.fit_valid = valid;
      }
    }
  }
}

TVector3 ParamSet::GetPosition() const {
  return TVector3(position_time.components[0].value, position_time.components[1].value,
                  position_time.components[2].value);
}

TVector3 ParamSet::GetDirection() const {
  TVector3 result =
      TVector3(direction.components[0].value, direction.components[1].value, direction.components[2].value);
  return result.Unit();
}

double ParamSet::GetTime() const { return position_time.components[3].value; }

double ParamSet::GetEnergy() const { return energy.components[0].value; }
}  // namespace Mimir
