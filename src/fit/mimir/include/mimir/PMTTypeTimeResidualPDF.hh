#pragma once
#include <Math/Interpolator.h>

#include <RAT/DS/PMTInfo.hh>
#include <mimir/Cost.hh>

namespace RAT::Mimir {
class PMTTypeTimeResidualPDF : public Cost {
 public:
  bool Configure(RAT::DBLinkPtr db_link) override;
  double operator()(const ParamSet& params) const override;

 protected:
  double group_velocity;
  double clamped_spline(const ROOT::Math::Interpolator& spline, double x) const;
  double left_bound, right_bound;
  std::map<int, ROOT::Math::Interpolator> tresid_nll_splines;
  RAT::DS::PMTInfo* pmt_info;
};

}  // namespace RAT::Mimir
