#include <TMath.h>

#include <RAT/NPEEstimator.hh>
#include <algorithm>
#include <cmath>
#include <vector>

namespace RAT {

size_t EstimateNPE(double charge, double vpe_charge, double npe_estimate_charge_width, size_t npe_estimate_max_pes) {
  std::vector<double> log_likelihood(npe_estimate_max_pes, 0.0);
  for (size_t npe = 1; npe <= npe_estimate_max_pes; ++npe) {
    log_likelihood[npe - 1] =
        -std::pow(charge - npe * vpe_charge, 2) / (2 * npe * std::pow(npe_estimate_charge_width, 2)) -
        0.5 * std::log(2 * TMath::Pi() * npe * std::pow(npe_estimate_charge_width, 2));
  }
  return std::distance(log_likelihood.begin(), std::max_element(log_likelihood.begin(), log_likelihood.end())) + 1;
}

}  // namespace RAT