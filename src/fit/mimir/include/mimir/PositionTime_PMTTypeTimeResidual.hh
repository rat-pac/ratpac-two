#pragma once
#include <mimir/FitStrategy.hh>
#include <mimir/PMTTypeTimeResidualPDF.hh>
#include <mimir/RootOptimizer.hh>

namespace RAT::Mimir {

class PositionTime_PMTTypeTimeResidual : public FitStrategy {
 public:
  PositionTime_PMTTypeTimeResidual(RAT::FitterInputHandler* input_handler, const std::string& name = "")
      : FitStrategy(input_handler, "PositionTime_PMTTypeTimeResidual", name),
        optimizer("PMTTypeTimeResidual"),
        cost_function() {}
  bool Configure(RAT::DBLinkPtr db_link) override;
  void Execute(ParamSet& params) override;

 protected:
  RootOptimizer optimizer;
  PMTTypeTimeResidualPDF cost_function;
};

}  // namespace RAT::Mimir
