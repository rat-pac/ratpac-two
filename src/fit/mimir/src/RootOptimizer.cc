#include <Math/Factory.h>
#include <Math/Functor.h>

#include <RAT/Log.hh>
#include <mimir/RootOptimizer.hh>
#include <sstream>

#include "Minuit2/Minuit2Minimizer.h"

namespace Mimir {

bool RootOptimizer::Configure(RAT::DBLinkPtr db_link) {
  std::string minimizer_type = db_link->GetS("minimizer_type");
  std::string algo_type = db_link->GetS("algo_type");
  fMinimizer = std::unique_ptr<ROOT::Math::Minimizer>(ROOT::Math::Factory::CreateMinimizer(minimizer_type, algo_type));
  fMinimizer->SetMaxFunctionCalls(db_link->GetI("max_function_calls"));
  fMinimizer->SetMaxIterations(db_link->GetI("max_iterations"));
  fMinimizer->SetTolerance(db_link->GetD("tolerance"));
  fMinimizer->SetPrintLevel(db_link->GetI("print_level"));
  RAT::info << "Mimir::RootOptimizer: Setting up the following optimizer: " << newline;
  std::stringstream minimizer_info;
  fMinimizer->Options().Print(minimizer_info);
  RAT::info << minimizer_info.str() << newline;
  return true;
}

void RootOptimizer::MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) {
  if (!fMinimizer) {
    RAT::Log::Die("Mimir::RootOptimizer: Minimizer is not configured!");
  }
  std::vector<double> fit_vector = params.to_active_vector();
  size_t n_params = fit_vector.size();
  auto wrapped_cost = [&](const double* xx) -> double {
    std::vector<double> xx_vec(xx, xx + n_params);
    ParamSet trial = params.from_active_vector(xx_vec);
    return cost(trial);
  };
  fMinimizer->Clear();
  ROOT::Math::Functor functor(wrapped_cost, n_params);
  fMinimizer->SetFunction(functor);
  std::vector<ParamComponent> active_components = params.to_active_components();
  for (size_t icomp = 0; icomp < n_params; ++icomp) {
    const ParamComponent& comp = active_components.at(icomp);
    bool has_lower_bound = comp.lower_bound > std::numeric_limits<double>::lowest();
    bool has_upper_bound = comp.upper_bound < std::numeric_limits<double>::max();
    if (has_lower_bound && has_upper_bound) {
      fMinimizer->SetLimitedVariable(icomp, comp.name, comp.value, comp.step, comp.lower_bound, comp.upper_bound);
    } else if (has_lower_bound) {
      fMinimizer->SetLowerLimitedVariable(icomp, comp.name, comp.value, comp.step, comp.lower_bound);
    } else if (has_upper_bound) {
      fMinimizer->SetUpperLimitedVariable(icomp, comp.name, comp.value, comp.step, comp.upper_bound);
    } else {
      fMinimizer->SetVariable(icomp, comp.name, comp.value, comp.step);
    }
  }
  fMinimizer->Minimize();
  std::vector<double> result(fMinimizer->X(), fMinimizer->X() + n_params);
  params = params.from_active_vector(result);
  params.set_active_fit_valid(fMinimizer->Status() == 0);
}
}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::Optimizer, Mimir::RootOptimizer, "RootOptimizer")
