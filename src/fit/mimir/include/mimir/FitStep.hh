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

  std::vector<ParamStatus> position_time_status;
  std::vector<ParamStatus> direction_status;
  std::vector<ParamStatus> energy_status;

  std::vector<double> position_time_lb;
  std::vector<double> position_time_ub;
  std::vector<double> direction_lb;
  std::vector<double> direction_ub;
  std::vector<double> energy_lb;
  std::vector<double> energy_ub;

  void set_status(RAT::DBLinkPtr db_link, const std::string& field_name, std::vector<ParamStatus>& status_vector);
  void set_bounds(RAT::DBLinkPtr db_link, const std::vector<std::string>& names, std::vector<double>& lb,
                  std::vector<double>& ub);
};

}  // namespace RAT::Mimir
