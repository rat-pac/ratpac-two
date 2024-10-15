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

namespace RAT {
namespace DS {

/** Processed waveform information **/
class DigitPMT : public TObject {
 public:
  DigitPMT() : TObject() {}
  virtual ~DigitPMT() {}

  /** ID number of PMT */
  virtual void SetID(Int_t _id) { this->id = _id; }
  virtual Int_t GetID() { return id; }

  /** Threshold crossing time in ns */
  virtual void SetDigitizedTime(Double_t _dTime) { this->dTime = _dTime - time_offset; }
  virtual Double_t GetDigitizedTime() { return dTime; }

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

  /** Fitted time in ns */
  virtual void SetFittedTime(Double_t _fTime) { this->fTime = _fTime; }
  virtual Double_t GetFittedTime() { return fTime; }

  /** Fitted mag */
  virtual void SetFittedHeight(Double_t _fMag) { this->fMag = _fMag; }
  virtual Double_t GetFittedHeight() { return fMag; }

  /** Fitted bas */
  virtual void SetFittedBaseline(Double_t _fBas) { this->fBas = _fBas; }
  virtual Double_t GetFittedBaseline() { return fBas; }

  /** Local trigger time at the location of the PMT. Useful for PMT timing corrections */
  virtual void SetLocalTriggerTime(Double_t _trigger_time) { this->local_trigger_time = _trigger_time; }
  virtual Double_t GetLocalTriggerTime() { return local_trigger_time; }

  /** Time offset applied to the waveform */
  virtual void SetTimeOffset(Double_t _time_offset) { this->time_offset = _time_offset; }
  virtual Double_t GetTimeOffset() { return time_offset; }

  /** Waveform analysis results */
  virtual WaveformAnalysisResult* const GetOrCreateWaveformAnalysisResult(std::string analyzer_name) {
    return &fit_results[analyzer_name];
  }

  virtual std::vector<std::string> const GetFitterNames() {
    std::vector<std::string> fitter_names;
    for (auto const& kv : fit_results) {
      fitter_names.push_back(kv.first);
    }
    return fitter_names;
  }

  ClassDef(DigitPMT, 5);

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
  Double_t local_trigger_time = -9999;
  Double_t time_offset = 0;
  std::map<std::string, WaveformAnalysisResult> fit_results;
};

}  // namespace DS
}  // namespace RAT

#endif
