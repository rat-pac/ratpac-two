#pragma once
#include <Math/Minimizer.h>

#include <mimir/Factory.hh>
#include <mimir/Optimizer.hh>
namespace RAT::Mimir {

class RootOptimizer : public Optimizer {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  void MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) override;

 protected:
  std::unique_ptr<ROOT::Math::Minimizer> fMinimizer = nullptr;
};
MIMIR_REGISTER_TYPE(Optimizer, RootOptimizer, "RootOptimizer")
}  // namespace RAT::Mimir
