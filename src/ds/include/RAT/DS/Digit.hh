/**
 * @class Digit
 * Data Structure: Digitized PMT in triggered event
 *
 * This represents a digitized PMT waveform in a detector event.
 */

#ifndef __RAT_DS_Digit__
#define __RAT_DS_Digit__

#include <TMath.h>
#include <TObject.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace RAT {
namespace DS {

class Digit : public TObject {
 public:
  Digit() : TObject() {}
  virtual ~Digit() {}

  // Digitizer name
  virtual void SetDigitName(std::string _name) { name = _name; };
  virtual std::string GetDigitName() const { return name; };

  // Sampling rate
  virtual void SetSamplingRate(Double_t _sampling_rate) { sampling_rate = _sampling_rate; };
  virtual Double_t GetSamplingRate() const { return sampling_rate; };

  // Total number of samples
  virtual void SetNSamples(uint32_t _nsamples) { nsamples = _nsamples; };
  virtual uint32_t GetNSamples() const { return nsamples; };

  // ADC bits
  virtual void SetNBits(UShort_t _nbits) { nbits = _nbits; };
  virtual UShort_t GetNBits() const { return nbits; };

  // Dynamic range (mV)
  virtual void SetDynamicRange(double _dynamic_range) { dynamic_range = _dynamic_range; };
  virtual Double_t GetDynamicRange() const { return dynamic_range; };

  // Termination ohms
  virtual void SetTerminationOhms(double _termination_ohms) { termination_ohms = _termination_ohms; };
  virtual Double_t GetTerminationOhms() const { return termination_ohms; };

  // Calculate time step (ns)
  virtual Double_t GetTimeStepNS() const { return 1.0 / sampling_rate; };

  // Calculate voltage resolution (mV)
  virtual Double_t GetVoltageResolution() const { return dynamic_range / pow(2, nbits); };

  /// Set a waveform, overwrites existing
  virtual void SetWaveform(const int waveformID, const std::vector<UShort_t> &samples) {
    waveforms[waveformID] = samples;
  }

  // Get a map of waveform IDs to digitized waveforms
  virtual std::map<int, std::vector<UShort_t>> GetAllWaveforms() const { return waveforms; }

  // Get the waveform for a digitizer
  virtual std::vector<UShort_t> GetWaveform(const int waveformID) const { return waveforms.at(waveformID); }

  /// Check if a waveform exists
  Bool_t ExistsWaveform(const int waveformID) const { return waveforms.count(waveformID) > 0; }

  /// Get a list (vector) of all the IDs that are available
  std::vector<int> GetIDs() const {
    std::vector<int> ret;
    for (std::map<int, std::vector<UShort_t>>::const_iterator it = waveforms.begin(); it != waveforms.end(); it++) {
      ret.push_back(it->first);
    }
    return ret;
  }

  /// Delete all waveforms
  virtual void PruneWaveforms() { waveforms.clear(); }

  ClassDef(Digit, 4);

 protected:
  std::string name;
  Double_t sampling_rate;
  uint32_t nsamples;
  UShort_t nbits;
  Double_t dynamic_range;
  Double_t termination_ohms;
  std::map<int, std::vector<UShort_t>> waveforms;  ///< Map of input number to samples
};

}  // namespace DS
}  // namespace RAT

#endif  // __RAT_DS_Digit__
