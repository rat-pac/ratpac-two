/**
 * @class WaveformAnalysisResult
 * Data Sturcture: Output of waveform fit results
 */

#ifndef __RAT_DS_WaveformAnalysisResult__
#define __RAT_DS_WaveformAnalysisResult__

#include <Rtypes.h>
#include <TObject.h>

#include <algorithm>
#include <map>
#include <string>

namespace RAT {
namespace DS {

class WaveformAnalysisResult : public TObject {
 public:
  virtual size_t AddPE(Double_t time, Double_t charge, std::map<std::string, Double_t> fom = {}) {
    size_t insertion_index = std::upper_bound(times.begin(), times.end(), time) - times.begin();
    times.insert(times.begin() + insertion_index, time);
    charges.insert(charges.begin() + insertion_index, charge);
    for (auto const& kv : fom) {
      std::vector<Double_t>& fom_array = figures_of_merit[kv.first];
      fom_array.insert(fom_array.begin() + insertion_index, kv.second);
    }
    return insertion_index;
  }
  virtual Double_t getTime(size_t idx) { return times.at(idx); }
  virtual Double_t getCharge(size_t idx) { return charges.at(idx); }
  virtual Double_t getFOM(std::string key, size_t idx) { return figures_of_merit.at(key).at(idx); }
  virtual int getNhits() { return times.size(); }
  ClassDef(WaveformAnalysisResult, 1);

 protected:
  // All arrays are parallel arrays. It is assumed that they are sorted in time
  std::vector<Double_t> times;
  std::vector<Double_t> charges;
  std::map<std::string, std::vector<Double_t>> figures_of_merit;
};

}  // namespace DS
}  // namespace RAT

#endif