#pragma once
#include <mimir/FitStrategy.hh>
#include <mimir/PMTTypeTimeResidualPDF.hh>
#include <mimir/RootOptimizer.hh>

#include "mimir/ParamSet.hh"

namespace RAT::Mimir {

class FitStep : public FitStrategy {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  void Execute(ParamSet& params) override;

 protected:
  std::unique_ptr<Optimizer> optimizer;
  std::unique_ptr<Cost> cost_function;

  std::vector<ParamStatus> position_time_status = {ParamStatus::INACTIVE, ParamStatus::INACTIVE, ParamStatus::INACTIVE,
                                                   ParamStatus::INACTIVE};
  std::vector<ParamStatus> direction_status = {ParamStatus::INACTIVE, ParamStatus::INACTIVE};
  std::vector<ParamStatus> energy_status = {ParamStatus::INACTIVE};

  void set_status(RAT::DBLinkPtr db_link, const std::string& field_name, std::vector<ParamStatus>& status_vector);
};

MIMIR_REGISTER_TYPE(FitStrategy, FitStep, "FitStep");

}  // namespace RAT::Mimir
