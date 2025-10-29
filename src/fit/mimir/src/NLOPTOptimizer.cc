#include <RAT/Config.hh>
#include <RAT/Log.hh>
#include <mimir/NLOPTOptimizer.hh>
#if NLOPT_Enabled
#include <nlopt.hpp>

namespace Mimir {

bool NLOPTOptimizer::Configure(RAT::DBLinkPtr db_link) {
  // Only allow gradient-free algorithms (LN_ or GN_)
  fAlgoType = db_link->GetS("algo_type");
  if (fAlgoType.size() >= 2 && fAlgoType[1] == 'D') {
    RAT::Log::Die("Mimir::NLOPTOptimizer: Gradient-based algorithm '" + fAlgoType +
                  "' is not supported. Please use a gradient-free (LN_ or GN_) algorithm.");
  }

  // Store remaining configuration parameters
  fMaxEval = db_link->GetI("max_function_calls");
  fTolerance = db_link->GetD("tolerance");

  // Print configuration summary
  RAT::info << "Mimir::NLOPTOptimizer: Setting up the following optimizer:" << newline;
  RAT::info << "  Algorithm: " << fAlgoType << newline;
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

  // Create the optimizer
  nlopt::opt optimizer;
  try {
    optimizer = nlopt::opt(fAlgoType.c_str(), n_params);
  } catch (const std::invalid_argument& e) {
    RAT::Log::Die("Mimir::NLOPTOptimizer: Invalid algorithm '" + fAlgoType + "': " + std::string(e.what()));
  } catch (const std::bad_alloc& e) {
    RAT::Log::Die("Mimir::NLOPTOptimizer: Memory allocation failed during optimizer creation: " +
                  std::string(e.what()));
  }

  // Set the configuration parameters
  optimizer.set_maxeval(fMaxEval);
  optimizer.set_xtol_rel(fTolerance);

  // Set up the objective function using lambda capture
  auto nlopt_wrapper = [&cost, &params](unsigned n, const double* x, double* grad) -> double {
    std::vector<double> x_vec(x, x + n);
    ParamSet trial = params.from_active_vector(x_vec);
    return cost(trial);
  };

  optimizer.set_min_objective(nlopt_wrapper);

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

  } catch (const std::bad_alloc& e) {
    RAT::Log::Die("Mimir::NLOPTOptimizer: Memory allocation failed during optimization: " + std::string(e.what()));
  } catch (const std::runtime_error& e) {
    RAT::Log::Die("Mimir::NLOPTOptimizer: Critical optimization error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    RAT::debug << "Mimir::NLOPTOptimizer: Optimization exception (fit marked invalid): " << e.what() << newline;
    params.set_active_fit_valid(false);
  }
}
}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::Optimizer, Mimir::NLOPTOptimizer, "NLOPTOptimizer")

#endif
