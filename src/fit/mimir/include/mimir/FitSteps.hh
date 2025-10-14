#pragma once
#include <mimir/Cost.hh>
#include <mimir/FitStrategy.hh>
#include <mimir/Optimizer.hh>

#include "mimir/ParamSet.hh"

namespace Mimir {

class FitSteps : public FitStrategy {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  void Execute(ParamSet& params) override;

  void SetInputHandler(RAT::FitterInputHandler* handler) override;

 protected:
  std::vector<std::unique_ptr<FitStrategy>> fSteps;
};

}  // namespace Mimir
