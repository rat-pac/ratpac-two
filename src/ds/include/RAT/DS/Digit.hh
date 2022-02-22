/**
 * @class Digit
 * Data Structure: Digitized PMT in triggered event
 * 
 * This represents a digitized PMT waveform in a detector event.
 */

#ifndef __RAT_DS_Digit__
#define __RAT_DS_Digit__

#include <TObject.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>

namespace RAT{
  namespace DS{

class Digit : public TObject {
public:
  Digit() : TObject() {}
  virtual ~Digit() {}

  // Digitizer name
  virtual void SetDigitName( std::string _name) { name = _name; };
  virtual std::string GetDigitName() const { return name; };

  // Sampling rate
  virtual void SetSamplingRate( Float_t _sampling_rate) { sampling_rate = _sampling_rate; };
  virtual Float_t GetSamplingRate() const { return sampling_rate; };

  // Total number of samples
  virtual void SetNSamples( UShort_t _nsamples) { nsamples = _nsamples; };
  virtual UShort_t GetNSamples() const { return nsamples; };

  // ADC bits
  virtual void SetNBits( UShort_t _nbits) { nbits = _nbits; };
  virtual UShort_t GetNBits() const { return nbits; };

  // Dynamic range (V)
  virtual void SetDynamicRange ( float _dynamic_range ) { dynamic_range = _dynamic_range; };
  virtual Float_t GetDynamicRange () const { return dynamic_range; };

  /// Set a waveform, overwrites existing
  virtual void SetWaveform( const UShort_t waveformID, const std::vector<UShort_t>& samples ) { waveforms[waveformID] = samples; }

  // Get a map of waveform IDs to digitized waveforms
  virtual std::map<UShort_t, std::vector<UShort_t> > GetAllWaveforms() const { return waveforms; }

  // Get the waveform for a digitizer
  virtual std::vector<UShort_t> GetWaveform( const UShort_t waveformID ) const { return waveforms.at( waveformID ); }

  /// Check if a waveform exists
  Bool_t ExistsWaveform( const UShort_t waveformID ) const { return waveforms.count( waveformID ) > 0; }

  /// Get a list (vector) of all the IDs that are available
  std::vector<UShort_t> GetIDs() const {
    std::vector<UShort_t> ret;
    for (std::map<UShort_t, std::vector<UShort_t> >::const_iterator it = waveforms.begin();
            it != waveforms.end(); it++){
      ret.push_back(it->first);
    }
    return ret;
  }

  /// Delete all waveforms
  virtual void PruneWaveforms() { waveforms.clear(); }

  ClassDef(Digit, 1);

protected:
  std::string name;
  Float_t sampling_rate;
  UShort_t nsamples;
  UShort_t nbits;
  Float_t dynamic_range; 
  std::map<UShort_t, std::vector<UShort_t> > waveforms; ///< Map of input number to samples

};

  }  // namespace DS
}  // namespace RAT

#endif  // __RAT_DS_Digit__
