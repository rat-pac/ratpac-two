/**
 * @class PMT
 * Data Structure: Output of analysis of digited PMT waveform
 *
 * This represents information about the digitzed PMT waveform in a detector event.
 */

#ifndef __RAT_DS_DigitPMT__
#define __RAT_DS_DigitPMT__

#include <Rtypes.h>
#include <TObject.h>

#include <RAT/DS/WaveformAnalysisResult.hh>
#include <RAT/Log.hh>
#include <limits>

namespace RAT {
namespace DS {

/** Processed waveform information **/
class DigitPMT : public TObject {
 public:
  typedef uint64_t HCMask;

  DigitPMT() : TObject() {}
  virtual ~DigitPMT() {}

  /** ID number of PMT */
  virtual void SetID(Int_t _id) { this->id = _id; }
  virtual Int_t GetID() { return id; }

  /** Threshold crossing time in ns */
  virtual void SetDigitizedTime(Double_t _dTime) { this->dTime = _dTime - time_offset; }
  virtual Double_t GetDigitizedTime() { return dTime; }
  virtual Double_t GetDigitizedTimeNoOffset() { return dTime + time_offset; }

  /** Integrated charge around the peak [pC] */
  virtual void SetDigitizedCharge(Double_t _dCharge) { this->dCharge = _dCharge; }
  virtual Double_t GetDigitizedCharge() { return dCharge; }

  /** Integrated total charge [pC] */
  virtual void SetDigitizedTotalCharge(Double_t _dTCharge) { this->dTCharge = _dTCharge; }
  virtual Double_t GetDigitizedTotalCharge() { return dTCharge; }

  /** Total number of threshold crossings */
  virtual void SetNCrossings(Int_t _nCrossings) { this->nCrossings = _nCrossings; }
  virtual Int_t GetNCrossings() { return nCrossings; }

  /** Total time that the voltage over threshold */
  virtual void SetTimeOverThreshold(Double_t _timeOverThresh) { this->timeOverThresh = _timeOverThresh; }
  virtual Double_t GetTimeOverThreshold() { return timeOverThresh; }

  /** Sum of the voltage over threshold */
  virtual void SetVoltageOverThreshold(Double_t _voltageOverThresh) { this->voltageOverThresh = _voltageOverThresh; }
  virtual Double_t GetVoltageOverThreshold() { return voltageOverThresh; }

  /** Baseline in a pre-trigger window */
  virtual void SetPedestal(Double_t _pedestal) { this->pedestal = _pedestal; }
  virtual Double_t GetPedestal() { return pedestal; }

  /** Peak voltage */
  virtual void SetPeakVoltage(Double_t _peak) { this->peak = _peak; }
  virtual Double_t GetPeakVoltage() { return peak; }

  /** Local trigger time at the location of the PMT. Useful for PMT timing corrections */
  virtual void SetLocalTriggerTime(Double_t _trigger_time) { this->local_trigger_time = _trigger_time; }
  virtual Double_t GetLocalTriggerTime() { return local_trigger_time; }

  /** Time offset applied to the waveform */
  virtual void SetTimeOffset(Double_t _time_offset) { this->time_offset = _time_offset; }
  virtual Double_t GetTimeOffset() { return time_offset; }

  /** Estimated number of photoelectrons, from likelihood algorithm */
  virtual void SetReconNPEs(Int_t _fNPE) { this->fNPE = _fNPE; }
  virtual Int_t GetReconNPEs() { return fNPE; }

  /** Waveform analysis results */
  virtual WaveformAnalysisResult* const GetOrCreateWaveformAnalysisResult(std::string analyzer_name) {
    if (!fit_results.count(analyzer_name)) {  // creating new fitresult
      if (fit_results[analyzer_name].getTimeOffset() != 0) {
        warn << "WavefornAnalysisResult for " << analyzer_name << " already has non-zero timing offset. "
             << "This should not happen.. Current value will override old value!" << newline;
        warn << "old value is " << fit_results[analyzer_name].getTimeOffset() << newline;
      }
      fit_results[analyzer_name].setTimeOffset(time_offset);
    }
    return &fit_results[analyzer_name];
  }

  virtual std::vector<std::string> const GetFitterNames() {
    std::vector<std::string> fitter_names;
    for (auto const& kv : fit_results) {
      fitter_names.push_back(kv.first);
    }
    return fitter_names;
  }

  /**
   * Set a bit in the hit cleaning mask.
   * @param bit_positon the literal bit position
   * @param val the value to write.
   */
  virtual void SetHitCleaningBit(uint bit_position, bool val = true) {
    if (bit_position > std::numeric_limits<HCMask>::digits - 1) {  // 0-indexing
      warn << "Tried to set bit out of hit cleaning bitmask range, ignoring." << newline;
      return;
    }
    hit_cleaning_mask = (hit_cleaning_mask & ~(1 << bit_position)) | (val << bit_position);
  }

  /**
   * Check a bit of the hit cleaning mask
   * @param bit_positon the literal bit position
   */
  virtual bool GetHitCleaningBit(uint bit_position) const {
    if (bit_position > std::numeric_limits<HCMask>::digits - 1) {  // 0-indexing
      warn << "Tried to read bit out of hit cleaning bitmask range, ignoring." << newline;
      return false;
    }
    HCMask mask = 1 << bit_position;
    return (hit_cleaning_mask & mask);
  }

  /** Retrieve hit cleaning mask */
  virtual HCMask GetHitCleaningMask() const { return hit_cleaning_mask; }
  virtual void SetHitCleaningMask(HCMask _hit_cleaning_mask) { hit_cleaning_mask = _hit_cleaning_mask; }

  ClassDef(DigitPMT, 7);

 protected:
  Int_t id = -9999;
  Double_t dTime = -9999;
  Double_t dCharge = -9999;
  Double_t dTCharge = -9999;
  Int_t nCrossings = -9999;
  Double_t timeOverThresh = -9999;
  Double_t voltageOverThresh = -9999;
  Double_t pedestal = -9999;
  Double_t peak = -9999;
  Double_t fTime = -9999;
  Double_t fMag = -9999;
  Double_t fBas = -9999;
  Int_t fNPE = 0;
  Double_t local_trigger_time = -9999;
  Double_t time_offset = 0;
  std::map<std::string, WaveformAnalysisResult> fit_results;
  HCMask hit_cleaning_mask = 0x0;
};

}  // namespace DS
}  // namespace RAT

#endif
