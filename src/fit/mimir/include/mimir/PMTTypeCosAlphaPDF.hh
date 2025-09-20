#pragma once
#include <Math/Interpolator.h>

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
  double left_bound, right_bound;
  double tresid_min, tresid_max;
  std::map<int, ROOT::Math::Interpolator> cosalpha_nll_splines;
  std::map<int, double> type_weights;
  RAT::DS::PMTInfo* pmt_info;

  double clamped_spline(const ROOT::Math::Interpolator& spline, double x) const;
};
}  // namespace Mimir
