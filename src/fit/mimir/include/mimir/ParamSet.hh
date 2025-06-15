#pragma once
#include <CLHEP/Units/PhysicalConstants.h>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace RAT::Mimir {
enum class ParamStatus { INACTIVE = 0, ACTIVE = 1, FIXED = 2 };

struct ParamComponent {
  std::string name;
  double value;
  double lower_bound = std::numeric_limits<double>::lowest();
  double upper_bound = std::numeric_limits<double>::max();
  double step = 0.01;
  ParamStatus status = ParamStatus::INACTIVE;

  bool is_active() const { return status == ParamStatus::ACTIVE; }
  bool is_fixed() const { return status == ParamStatus::FIXED; }
  bool is_used() const { return status != ParamStatus::INACTIVE; }
};

struct ParamField {
  std::vector<ParamComponent> components;
  std::vector<double> active_values() const;
  std::vector<double> used_values() const;
  std::vector<double> active_lower_bounds() const;
  std::vector<double> active_upper_bounds() const;
  void set_all_lower_bounds(double value);
  void set_all_upper_bounds(double value);
  void set_all_status(ParamStatus status);
  void set_status(std::vector<ParamStatus> status_vector);
  void set_values(std::vector<double> values);
  bool are_all_used() const {
    return std::all_of(components.begin(), components.end(), [](const ParamComponent& comp) { return comp.is_used(); });
  }
  bool are_all_active() const {
    return std::all_of(components.begin(), components.end(),
                       [](const ParamComponent& comp) { return comp.is_active(); });
  }
};

struct ParamSet {
  ParamField position_time = {.components = {{.name = "x", .value = 0.0},
                                             {.name = "y", .value = 0.0},
                                             {.name = "z", .value = 0.0},
                                             {.name = "t", .value = 0.0}}};
  ParamField direction = {.components = {
                              {.name = "theta", .value = CLHEP::pi / 2, .lower_bound = 0, .upper_bound = CLHEP::pi},
                              {.name = "phi", .value = 0.0, .lower_bound = -CLHEP::pi, .upper_bound = CLHEP::pi},
                          }};
  ParamField energy = {.components = {{.name = "energy", .value = 1.0, .lower_bound = 0.0}}};

  std::vector<double> to_active_vector() const;
  std::vector<ParamComponent> to_active_components() const;
  void update_active(const std::vector<double>& values);
  ParamSet from_active_vector(const std::vector<double>& values) const;
};

}  // namespace RAT::Mimir
