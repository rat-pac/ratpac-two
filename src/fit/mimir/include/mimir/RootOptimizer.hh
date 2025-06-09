#pragma once
#include <Math/Minimizer.h>

#include <mimir/Optimizer.hh>
namespace RAT::Mimir {

class RootOptimizer : public Optimizer {
 public:
  RootOptimizer() : Optimizer("Minuit") {}
  RootOptimizer(const std::string& name) : Optimizer("Minuit", name) {}
  bool Configure(RAT::DBLinkPtr db_link) override;
  void MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) override;

 protected:
  std::unique_ptr<ROOT::Math::Minimizer> fMinimizer = nullptr;
};

}  // namespace RAT::Mimir
