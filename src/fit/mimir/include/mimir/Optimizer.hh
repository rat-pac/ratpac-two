#pragma once
#include <RAT/DB.hh>
#include <functional>
#include <mimir/Cost.hh>
#include <mimir/ParamSet.hh>

namespace RAT::Mimir {

class Optimizer {
 public:
  Optimizer() = default;
  Optimizer(const std::string& type, const std::string& name = "") : type_(type), name_(name) {}
  virtual ~Optimizer() = default;

  // Configure the optimizer with a database link
  virtual bool Configure(RAT::DBLinkPtr db_link) { return true; }

  // Optimize the given cost function with the provided parameters
  void Minimize(const Cost& cost, ParamSet& params) {
    return MinimizeImpl([&cost](const ParamSet& p) { return cost(p); }, params);
  }

  void Maximize(const Cost& cost, ParamSet& params) {
    return MinimizeImpl([&cost](const ParamSet& p) { return -cost(p); }, params);
  }

  virtual void MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) = 0;

  // Get the name of the optimizer
  const std::string& Name() const { return name_; }

  // Get the type of the optimizer
  const std::string Type() const { return type_; }

 protected:
  const std::string type_;
  const std::string name_;
};

}  // namespace RAT::Mimir
