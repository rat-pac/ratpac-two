#include <RAT/Config.hh>
#if NLOPT_Enabled
#pragma once
#include <mimir/Factory.hh>
#include <mimir/Optimizer.hh>
#include <nlopt.hpp>
namespace Mimir {

class NLOPTOptimizer : public Optimizer {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  void MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) override;

 protected:
  nlopt::algorithm fAlgorithm;
  int fMaxEval;
  double fTolerance;
};
}  // namespace Mimir
#endif
