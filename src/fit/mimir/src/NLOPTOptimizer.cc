#include <RAT/Log.hh>
#include <limits>
#include <mimir/NLOPTOptimizer.hh>
#include <nlopt.hpp>

namespace Mimir {

bool NLOPTOptimizer::Configure(RAT::DBLinkPtr db_link) {
  std::string algo_type = db_link->GetS("algo_type");

  // Parse and validate the algorithm string using the NLopt C API
  nlopt_algorithm algo_c = nlopt_algorithm_from_string(algo_type.c_str());
  if (algo_c == NLOPT_NUM_ALGORITHMS) {
    RAT::Log::Die(std::string("Mimir::NLOPTOptimizer: Unknown or unsupported algorithm '") + algo_type + "'.");
  }
  // Only allow gradient-free algorithms (LN_ or GN_)
  if (algo_type.rfind("D_", 0) == 0) {
    RAT::Log::Die(std::string("Mimir::NLOPTOptimizer: Gradient-based algorithm '") + algo_type +
                  "' is not supported. Please use a gradient-free (LN_ or GN_) algorithm.");
  }
  fAlgorithm = static_cast<nlopt::algorithm>(algo_c);

  // Store remaining configuration parameters
  fMaxEval = db_link->GetI("max_function_calls");
  fTolerance = db_link->GetD("tolerance");

  // Print configuration summary (match RootOptimizer style)
  RAT::info << "Mimir::NLOPTOptimizer: Setting up the following optimizer:" << newline;
  RAT::info << "  Algorithm: " << algo_type << newline;
  RAT::info << "  Max function calls: " << fMaxEval << newline;
  RAT::info << "  Tolerance: " << fTolerance << newline;
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

  // Set up the objective function
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

  // Set bounds for each parameter
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

  // Run the optimization
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

    RAT::debug << "Mimir::NLOPTOptimizer: Optimization completed with result code " << nlopt_result
               << ", final cost: " << final_cost << newline;

  } catch (const std::exception& e) {
    RAT::warn << "Mimir::NLOPTOptimizer: Optimization failed: " << e.what() << newline;
    params.set_active_fit_valid(false);
  }
}
}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::Optimizer, Mimir::NLOPTOptimizer, "NLOPTOptimizer")
