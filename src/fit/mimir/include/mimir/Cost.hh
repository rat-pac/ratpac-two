#pragma once
#include <RAT/DB.hh>
#include <RAT/FitterInputHandler.hh>
#include <RAT/Log.hh>
#include <mimir/Common.hh>
#include <mimir/ParamSet.hh>

namespace Mimir {

class Cost {
 public:
  Cost() = default;
  virtual bool Configure(RAT::DBLinkPtr db_link) = 0;
  virtual double operator()(const ParamSet& params) const = 0;

  virtual void AddHits(const int pmtid, const RAT::FitterInputHandler& input_handler) {
    std::vector<double> times = input_handler.GetTimes(pmtid);
    std::vector<double> charges = input_handler.GetCharges(pmtid);
    if (times.size() != charges.size()) {
      RAT::Log::Die("mimir::Cost: GetTimes and GetCharges returned vectors of different sizes for PMT " +
                    std::to_string(pmtid) + ".");
    }
    for (size_t i = 0; i < times.size(); ++i) {
      hit_pmtids.push_back(pmtid);
      hit_times.push_back(times[i]);
      hit_charges.push_back(charges[i]);
    }
  }

  virtual void AddAllHits(const RAT::FitterInputHandler& input_handler) {
    std::vector<int> pmtids = input_handler.GetAllHitPMTIDs();
    for (int pmtid : pmtids) {
      AddHits(pmtid, input_handler);
    }
  }

  virtual void ClearHits() {
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
