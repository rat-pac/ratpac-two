#include <RAT/Log.hh>
#include <limits>
#include <mimir/NLOPTOptimizer.hh>
#include <nlopt.hpp>

namespace Mimir {

bool NLOPTOptimizer::Configure(RAT::DBLinkPtr db_link) {
  std::string algo_type = db_link->GetS("algo_type");

  // Map algorithm string to NLopt algorithm enum (derivative-free only)
  nlopt::algorithm algorithm = nlopt::LN_NELDERMEAD;  // Default to derivative-free
  if (algo_type == "LN_COBYLA")
    algorithm = nlopt::LN_COBYLA;
  else if (algo_type == "LN_BOBYQA")
    algorithm = nlopt::LN_BOBYQA;
  else if (algo_type == "LN_NEWUOA")
    algorithm = nlopt::LN_NEWUOA;
  else if (algo_type == "LN_NELDERMEAD")
    algorithm = nlopt::LN_NELDERMEAD;
  else if (algo_type == "LN_SBPLX")
    algorithm = nlopt::LN_SBPLX;
  else {
    // If a gradient-based algorithm was requested, fall back and warn
    if (algo_type == "LD_MMA" || algo_type == "LD_CCSAQ" || algo_type == "LD_SLSQP" || algo_type == "LD_LBFGS") {
      RAT::warn << "Mimir::NLOPTOptimizer: Gradient-based algorithm '" << algo_type
                << "' requested; gradients are disabled. Falling back to LN_NELDERMEAD." << newline;
    } else {
      RAT::warn << "Mimir::NLOPTOptimizer: Unknown algorithm '" << algo_type << "', using LN_NELDERMEAD" << newline;
    }
  }

  // Store configuration parameters - minimizer will be created in MinimizeImpl
  fAlgorithm = algorithm;
  fMaxEval = db_link->GetI("max_function_calls");
  fTolerance = db_link->GetD("tolerance");

  RAT::debug << "Mimir::NLOPTOptimizer: Configured with algorithm " << algo_type
             << ", max_function_calls=" << db_link->GetI("max_function_calls")
             << ", tolerance=" << db_link->GetD("tolerance");
  RAT::debug << newline;
  return true;
}

void NLOPTOptimizer::MinimizeImpl(std::function<double(const ParamSet&)> cost, ParamSet& params) {
  std::vector<double> fit_vector = params.to_active_vector();
  size_t n_params = fit_vector.size();

  if (n_params == 0) {
    RAT::warn << "Mimir::NLOPTOptimizer: No active parameters to optimize!" << newline;
    return;
  }

  // Create the optimizer with the correct dimension using stored configuration
  nlopt::opt optimizer = nlopt::opt(fAlgorithm, n_params);

  // Set the configuration parameters
  optimizer.set_maxeval(fMaxEval);
  optimizer.set_xtol_rel(fTolerance);

  // Create a wrapper function for NLopt (no gradients)
  struct OptData {
    std::function<double(const ParamSet&)> cost_func;
    ParamSet* param_template;
  };

  OptData opt_data{cost, &params};

  auto nlopt_wrapper = [](const std::vector<double>& x, std::vector<double>& /*grad*/, void* data) -> double {
    OptData* opt_data_ptr = static_cast<OptData*>(data);
    ParamSet trial = opt_data_ptr->param_template->from_active_vector(x);
    return opt_data_ptr->cost_func(trial);
  };

  optimizer.set_min_objective(nlopt_wrapper, &opt_data);

  // Set bounds
  std::vector<ParamComponent> active_components = params.to_active_components();
  std::vector<double> lower_bounds, upper_bounds;

  for (size_t icomp = 0; icomp < n_params; ++icomp) {
    const ParamComponent& comp = active_components.at(icomp);

    double lb = comp.lower_bound;
    double ub = comp.upper_bound;

    // Handle infinite bounds
    if (lb <= std::numeric_limits<double>::lowest()) {
      lb = -HUGE_VAL;
    }
    if (ub >= std::numeric_limits<double>::max()) {
      ub = HUGE_VAL;
    }

    lower_bounds.push_back(lb);
    upper_bounds.push_back(ub);
  }

  optimizer.set_lower_bounds(lower_bounds);
  optimizer.set_upper_bounds(upper_bounds);

  // Run optimization
  std::vector<double> result = fit_vector;
  double final_cost;

  try {
    nlopt::result nlopt_result = optimizer.optimize(result, final_cost);

    // Update parameters with result
    params = params.from_active_vector(result);

    // Set fit validity based on NLopt result
    bool fit_valid = (nlopt_result == nlopt::SUCCESS || nlopt_result == nlopt::STOPVAL_REACHED ||
                      nlopt_result == nlopt::FTOL_REACHED || nlopt_result == nlopt::XTOL_REACHED);
    params.set_active_fit_valid(fit_valid);

    RAT::debug << "Mimir::NLOPTOptimizer: Optimization completed with result " << nlopt_result
               << ", final cost: " << final_cost << newline;

  } catch (const std::exception& e) {
    RAT::warn << "Mimir::NLOPTOptimizer: Optimization failed: " << e.what() << newline;
    params.set_active_fit_valid(false);
  }
}
}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::Optimizer, Mimir::NLOPTOptimizer, "NLOPTOptimizer")
