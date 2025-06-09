#include <Math/Factory.h>
#include <Math/Functor.h>

#include <RAT/Log.hh>
#include <mimir/RootOptimizer.hh>
#include <sstream>

namespace RAT::Mimir {

bool RootOptimizer::Configure(RAT::DBLinkPtr db_link) {
  // TODO:: configurable minimizers and algorithms.
  fMinimizer = std::unique_ptr<ROOT::Math::Minimizer>(ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad"));
  fMinimizer->SetMaxFunctionCalls(10000);
  fMinimizer->SetMaxIterations(1000);
  fMinimizer->SetTolerance(1e-4);
  fMinimizer->SetPrintLevel(1);
  info << "Mimir::RootOptimizer: Setting up the following optimizer: " << newline;
  std::stringstream minimizer_info;
  fMinimizer->Options().Print(minimizer_info);
  info << minimizer_info.str() << newline;
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
}

}  // namespace RAT::Mimir
