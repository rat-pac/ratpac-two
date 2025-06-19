#include <TVector3.h>

#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <mimir/PMTTypeTimeResidualPDF.hh>

#include "Math/Interpolator.h"

namespace RAT::Mimir {
bool PMTTypeTimeResidualPDF::Configure(RAT::DBLinkPtr db_link) {
  std::vector<double> binning = db_link->GetDArray("binning");
  std::vector<int> pmt_types = db_link->GetIArray("pmt_types");
  pmt_info = RAT::DS::RunStore::GetCurrentRun()->GetPMTInfo();
  left_bound = binning.front();
  right_bound = binning.back();
  for (int pmt_type : pmt_types) {
    std::vector<double> histvals = db_link->GetDArray("hist_" + std::to_string(pmt_type));
    std::vector<double> nll_vals;
    for (const auto& val : histvals) {
      nll_vals.push_back(-std::log(val));
    }
    tresid_nll_splines.try_emplace(pmt_type, binning, nll_vals, ROOT::Math::Interpolation::kCSPLINE);
  }
  group_velocity = db_link->GetD("group_velocity");
  return true;
}

double PMTTypeTimeResidualPDF::operator()(const ParamSet& params) const {
  if (hit_pmtids.size() != hit_times.size() || hit_pmtids.size() != hit_charges.size()) {
    RAT::Log::Die("mimir::PMTTypeTimeResidualPDF: hit_pmtids, hit_times, and hit_charges must have the same size.");
  }
  if (hit_pmtids.empty()) {
    RAT::warn << "mimir::PMTTypeTimeResidualPDF: No hits set, returning 0." << newline;
    return 0.0;
  }
  if (hit_times.at(0) == INVALID) {
    RAT::Log::Die(
        "mimir::PMTTypeTimeResidualPDF: hit times were not provided. Cannot evaluate Time Residual PDF without hit "
        "times.");
  }
  ParamField position_time = params.position_time;
  std::vector<double> xyzt = position_time.active_values();
  if (xyzt.size() != 4) {
    RAT::Log::Die("mimir::PMTTypeTimeResidualPDF: position_time must have 4 active components (x, y, z, t).");
  }
  TVector3 vertex_position(xyzt.at(0), xyzt.at(1), xyzt.at(2));
  double vertex_time = xyzt.at(3);
  double result = 0.0;
  for (size_t ihit = 0; ihit < hit_pmtids.size(); ++ihit) {
    int pmtid = hit_pmtids.at(ihit);
    double time = hit_times.at(ihit);
    int pmt_type = pmt_info->GetModel(pmtid);
    TVector3 pmt_position = pmt_info->GetPosition(pmtid);
    const ROOT::Math::Interpolator& spline_to_use = tresid_nll_splines.at(pmt_type);
    double tof = (vertex_position - pmt_position).Mag() / group_velocity;
    double tresid = time - vertex_time - tof;
    result += clamped_spline(spline_to_use, tresid);
  }

  return result;
}

double PMTTypeTimeResidualPDF::clamped_spline(const ROOT::Math::Interpolator& spline, double x) const {
  if (x <= left_bound) {
    return spline.Eval(left_bound + 1e-4);
  } else if (x >= right_bound) {
    return spline.Eval(right_bound - 1e-4);
  } else {
    return spline.Eval(x);
  }
}

}  // namespace RAT::Mimir
