#include <TVector3.h>

#include <RAT/DS/RunStore.hh>
#include <RAT/Log.hh>
#include <mimir/PMTTypeCosAlphaPDF.hh>
#include <numeric>

#include "Math/Interpolator.h"

namespace Mimir {
bool PMTTypeCosAlphaPDF::Configure(RAT::DBLinkPtr db_link) {
  std::vector<double> binning = db_link->GetDArray("binning");
  std::vector<int> pmt_types = db_link->GetIArray("pmt_types");
  std::vector<double> _type_weights = db_link->GetDArray("type_weights");
  RAT::Log::Assert(pmt_types.size() == _type_weights.size(),
                   "mimir::PMTTypeCosAlphaPDF: pmt_types and type_weights must have the same size.");
  pmt_info = RAT::DS::RunStore::GetCurrentRun()->GetPMTInfo();
  double bin_width = binning.at(1) - binning.at(0);
  cosalpha_nll_splines.clear();
  type_weights.clear();
  for (size_t idx = 0; idx < pmt_types.size(); ++idx) {
    int pmt_type = pmt_types.at(idx);
    type_weights[pmt_type] = _type_weights.at(idx);
    std::vector<double> histvals = db_link->GetDArray("hist_" + std::to_string(pmt_type));
    double norm = std::accumulate(histvals.begin(), histvals.end(), 0.0) * bin_width;
    std::vector<double> nll_vals;
    for (const auto& val : histvals) {
      if (val <= 0) {
        RAT::Log::Die("mimir::PMTTypeCosAlphaPDF: PDF histogram for PMT type " + std::to_string(pmt_type) +
                      " has zero or negative bin content, cannot take log.");
      }
      nll_vals.push_back(-std::log(val / norm));
    }
    cosalpha_nll_splines.try_emplace(pmt_type, binning, nll_vals, ROOT::Math::Interpolation::kCSPLINE);
  }

  light_speed_in_medium = db_link->GetD("light_speed_in_medium");
  try {
    std::vector<double> tresid_range = db_link->GetDArray("tresid_range");
    tresid_min = tresid_range.at(0);
    tresid_max = tresid_range.at(1);
  } catch (RAT::DBNotFoundError&) {
    tresid_min = -std::numeric_limits<double>::infinity();
    tresid_max = std::numeric_limits<double>::infinity();
  }

  return true;
}

double PMTTypeCosAlphaPDF::operator()(const ParamSet& params) const {
  if (hit_pmtids.size() != hit_times.size() || hit_pmtids.size() != hit_charges.size()) {
    RAT::Log::Die("mimir::PMTTypeCosAlphaPDF: hit_pmtids, hit_times, and hit_charges must have the same size.");
  }
  if (hit_pmtids.empty()) {
    RAT::warn << "mimir::PMTTypeCosAlphaPDF: No hits set, returning 0." << newline;
    return 0.0;
  }
  if (hit_times.at(0) == INVALID) {
    RAT::Log::Die(
        "mimir::PMTTypeCosAlphaPDF: hit times were not provided. Cannot evaluate Time Residual PDF without hit "
        "times.");
  }
  if (!params.direction.are_all_active()) {
    RAT::Log::Die("mimir::PMTTypeCosAlphaPDF: not all direction comoponents are active.");
  }
  TVector3 vertex_direction = params.GetDirection();
  if (!params.position_time.are_all_used())
    RAT::Log::Die("mimir::PMTTypeCosAlphaPDF: position_time must have a valid position vertex (x, y, z, t).");
  TVector3 vertex_position = params.GetPosition();
  double vertex_time = params.GetTime();

  double result = 0.0;
  for (size_t ihit = 0; ihit < hit_pmtids.size(); ++ihit) {
    int pmtid = hit_pmtids.at(ihit);
    double time = hit_times.at(ihit);
    TVector3 pmt_position = pmt_info->GetPosition(pmtid);
    TVector3 to_pmt = (pmt_position - vertex_position);
    double tof = to_pmt.Mag() / light_speed_in_medium;
    double tresid = time - vertex_time - tof;
    if (tresid < tresid_min || tresid > tresid_max) {
      continue;
    }
    double cosalpha = vertex_direction.Dot(to_pmt.Unit());
    int pmt_type = pmt_info->GetType(pmtid);
    if (cosalpha_nll_splines.count(pmt_type) == 0) {
      RAT::warn << "mimir::PMTTypeCosAlphaPDF: No spline for PMT type " << pmt_type << ". Skipping this hit."
                << newline;
      continue;
    }
    const RAT::BoundedInterpolator& spline_to_use = cosalpha_nll_splines.at(pmt_type);
    double weight = type_weights.at(pmt_type);
    result += spline_to_use.Eval(cosalpha) * weight;
  }

  return result;
}

}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::Cost, Mimir::PMTTypeCosAlphaPDF, "PMTTypeCosAlphaPDF")
