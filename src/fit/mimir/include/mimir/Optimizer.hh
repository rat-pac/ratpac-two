#pragma once
#include <RAT/DB.hh>
#include <functional>
#include <mimir/Cost.hh>
#include <mimir/ParamSet.hh>

namespace Mimir {

class Optimizer {
 public:
  Optimizer() = default;
  virtual ~Optimizer() = default;

  // Configure the optimizer with a database link
  virtual bool Configure(RAT::DBLinkPtr db_link) { return true; }

  // Optimize the given cost function with the provided parameters
  void Minimize(const Cost* cost, ParamSet& params) {
    return MinimizeImpl([cost](const ParamSet& p) { return (*cost)(p); }, params);
  }

  void Maximize(const Cost* cost, ParamSet& params) {
    return MinimizeImpl([cost](const ParamSet& p) { return -(*cost)(p); }, params);
  }

  virtual void MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) = 0;

  const std::string& GetName() const { return name; }

  void SetName(const std::string& _name) { name = _name; }

 protected:
  std::string name;
};

}  // namespace Mimir
