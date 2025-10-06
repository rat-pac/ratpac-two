#pragma once
#include <RAT/BoundedInterpolator.hh>
#include <RAT/DS/PMTInfo.hh>
#include <mimir/Cost.hh>
#include <mimir/Factory.hh>

namespace Mimir {
class PMTTypeCosAlphaPDF : public Cost {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  double operator()(const ParamSet& params) const override;

 protected:
  double light_speed_in_medium;
  double tresid_min, tresid_max;
  std::map<int, RAT::BoundedInterpolator> cosalpha_nll_splines;
  std::map<int, double> type_weights;
  RAT::DS::PMTInfo* pmt_info;
};
}  // namespace Mimir
