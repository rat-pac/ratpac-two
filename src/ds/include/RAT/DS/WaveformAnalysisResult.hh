/**
 * @class WaveformAnalysisResult
 * Data Sturcture: Output of waveform fit results
 */

#ifndef __RAT_DS_WaveformAnalysisResult__
#define __RAT_DS_WaveformAnalysisResult__

#include <Rtypes.h>
#include <TObject.h>

#include <RAT/Log.hh>
#include <algorithm>
#include <map>
#include <string>

namespace RAT {
namespace DS {

class WaveformAnalysisResult : public TObject {
 public:
  /* *
   * Add information about a single reconstructed pulse.
   * @param time   The time of the pulse. Within the digitization window. No shifting due to cable delay or trigger
   *               should be applied here.
   * @param charge charge the correspond to the pulse. Nominally in units of pC.
   * @param fom    map of all figures of merits. key should be name of the field (e.g. chi2ndf)
   * */
  virtual size_t AddPE(Double_t time, Double_t charge, std::map<std::string, Double_t> fom = {}) {
    Double_t time_corrected = time - time_offset;
    size_t insertion_index = std::upper_bound(times.begin(), times.end(), time_corrected) - times.begin();
    times.insert(times.begin() + insertion_index, time_corrected);
    charges.insert(charges.begin() + insertion_index, charge);
    for (auto const& kv : fom) {
      std::vector<Double_t>& fom_array = figures_of_merit[kv.first];
      fom_array.insert(fom_array.begin() + insertion_index, kv.second);
    }
    return insertion_index;
  }
  virtual void setTimeOffset(Double_t _offset) { time_offset = _offset; }
  virtual Double_t getTimeOffset() { return time_offset; }
  virtual Double_t getTime(size_t idx) { return times.at(idx); }
  virtual Double_t getCharge(size_t idx) { return charges.at(idx); }
  virtual std::vector<std::string> getFOMNames() {  // Function to retrive the names of figures of merit
    std::vector<std::string> FOMNames;
    FOMNames.reserve(figures_of_merit.size());
    for (const auto& pair : figures_of_merit) {
      FOMNames.push_back(pair.first);
    }
    return FOMNames;
  }
  virtual Double_t getFOMValue(std::string key, size_t idx) { return figures_of_merit.at(key).at(idx); }
  virtual int getNhits() { return times.size(); }
  ClassDef(WaveformAnalysisResult, 1);

 protected:
  // All arrays are parallel arrays. It is assumed that they are sorted in time
  std::vector<Double_t> times;
  std::vector<Double_t> charges;
  std::map<std::string, std::vector<Double_t>> figures_of_merit;
  Double_t time_offset;
};

}  // namespace DS
}  // namespace RAT

#endif
