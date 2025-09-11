#pragma once
#include <RAT/DB.hh>
#include <mimir/Common.hh>
#include <mimir/ParamSet.hh>

namespace Mimir {

class Cost {
 public:
  Cost() = default;
  virtual bool Configure(RAT::DBLinkPtr db_link) = 0;
  virtual double operator()(const ParamSet& params) const = 0;

  void SetHits(const std::vector<int>& pmtids, const std::vector<double>& _times = {},
               const std::vector<double>& _charges = {}) {
    std::vector<double> times = _times;
    std::vector<double> charges = _charges;
    // allow these values to be empty so that cost functions can be evaluated on just hit channels.
    if (!_times.size()) times.resize(pmtids.size(), INVALID);
    if (!_charges.size()) charges.resize(pmtids.size(), INVALID);
    hit_pmtids = pmtids;
    hit_times = times;
    hit_charges = charges;
  }

  void AddHit(const int pmtid, const double time = INVALID, const double charge = INVALID) {
    hit_pmtids.push_back(pmtid);
    hit_times.push_back(time);
    hit_charges.push_back(charge);
  }

  void ClearHits() {
    hit_pmtids.clear();
    hit_times.clear();
    hit_charges.clear();
  }

  const std::string GetName() const { return name; }

  void SetName(const std::string& _name) { name = _name; }

 protected:
  std::vector<int> hit_pmtids;
  std::vector<double> hit_times;
  std::vector<double> hit_charges;
  std::string name;
};

}  // namespace Mimir
