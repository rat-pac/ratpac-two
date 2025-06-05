/**
 * @class DS::EV
 * Data Structure: Triggered event
 *
 * @author Stan Seibert <sseibert@hep.upenn.edu>
 *
 * This class represents a detected event as defined by the trigger
 * simulation.
 */

#ifndef __RAT_DS_EV__
#define __RAT_DS_EV__

// Clang 5.0 headers confuse rootcint...
#ifdef __MAKECINT__
#define __signed signed
#endif

#include <TObject.h>
#include <TTimeStamp.h>

#include <RAT/DS/Digit.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/LAPPD.hh>
#include <RAT/DS/PMT.hh>
#include <limits>
#include <vector>

namespace RAT {
namespace DS {

class EV : public TObject {
 public:
  EV() : TObject() {}
  virtual ~EV() {}

  /** Event number. */
  virtual Int_t GetID() const { return id; }
  virtual void SetID(Int_t _id) { id = _id; }

  /** Date/time of event trigger (UTC)*/
  virtual TTimeStamp GetUTC() const { return utc; }
  virtual void SetUTC(const TTimeStamp &_utc) { utc = _utc; }

  /** Trigger Word. Meaning depends on DAQ implementation */
  virtual uint64_t GetTriggerWord() const { return trigger_word; }
  virtual void SetTriggerWord(const uint64_t &_trigger_word) { trigger_word = _trigger_word; }

  /** List of pmts with at least one charge sample in this event. */
  virtual PMT *GetOrCreatePMT(Int_t id) {
    pmt[id].SetID(id);
    return &pmt[id];
  }
  const std::vector<Int_t> GetAllPMTIDs() {
    std::vector<Int_t> result;
    for (auto const &kv : pmt) {
      result.push_back(kv.first);
    }
    return result;
  }
  virtual Int_t GetPMTCount() const { return pmt.size(); }
  virtual void PrunePMT() { pmt.clear(); }

  /** List of pmts with at least one charge sample in this event. */
  virtual DigitPMT *GetOrCreateDigitPMT(Int_t id) {
    digitpmt[id].SetID(id);
    return &digitpmt[id];
  }
  const std::vector<Int_t> GetAllDigitPMTIDs() {
    std::vector<Int_t> result;
    for (auto const &kv : digitpmt) {
      result.push_back(kv.first);
    }
    return result;
  }
  virtual size_t EraseDigitPMT(Int_t id) {
    size_t n_erased = digitpmt.erase(id);
    return n_erased;
  }
  virtual Int_t GetDigitPMTCount() const { return digitpmt.size(); }
  virtual void PruneDigitPMT() { digitpmt.clear(); }

  const std::vector<Int_t> GetAllCleanedDigitPMTIDs() const {
    std::vector<Int_t> result;
    for (std::pair<Int_t, DigitPMT> kv : digitpmt) {
      DigitPMT::HCMask hit_cleaning_mask = kv.second.GetHitCleaningMask();
      if (hit_cleaning_mask == 0) {
        result.push_back(kv.first);
      }
    }
    return result;
  }

  // TODO: Implemetation for PMT class
  const std::vector<Int_t> GetAllCleanedPMTIDs() const {
    std::vector<Int_t> result;
    return result;
  }

  /** Number of PMTs which were hit at least once. (Convenience method) */
  virtual Int_t Nhits() const { return GetPMTCount(); }
  virtual Int_t cleanedNhits() const { return GetAllCleanedPMTIDs().size(); }
  virtual Int_t cleanedDigitNhits() const { return GetAllCleanedDigitPMTIDs().size(); }

  /** List of LAPPDs with at least one charge sample in this event. */
  virtual LAPPD *GetLAPPD(Int_t i) { return &lappd[i]; }
  virtual Int_t GetLAPPDCount() const { return lappd.size(); }
  virtual LAPPD *AddNewLAPPD() {
    lappd.resize(lappd.size() + 1);
    return &lappd.back();
  }
  virtual void PruneLAPPD() { lappd.resize(0); }

  /** Time since last trigger in ns. */
  Double_t GetDeltaT() const { return deltat; }
  void SetDeltaT(Double_t _deltat) { deltat = _deltat; }

  void SetCalibratedTriggerTime(Double_t _calibratedTriggerTime) { calibratedTriggerTime = _calibratedTriggerTime; }
  Double_t GetCalibratedTriggerTime() const { return calibratedTriggerTime; }

  /** Total charge in all PMT waveforms (pC). */
  Double_t GetTotalCharge() const { return qTotal; }
  void SetTotalCharge(Double_t _qTotal) { qTotal = _qTotal; }

  /** Fit Results **/
  virtual std::vector<FitResult *> GetFitResults() { return fitResults; }
  virtual void AddFitResult(FitResult *fit) { fitResults.push_back(fit); }
  virtual void PruneFitResults() { fitResults.resize(0); }

  /** Classifier Results **/
  virtual std::vector<Classifier *> GetClassifierResults() { return classifierResults; }
  virtual void AddClassifierResult(Classifier *clf) { classifierResults.push_back(clf); }
  virtual void PruneClassifierResults() { classifierResults.resize(0); }

  /// Set CAEN digitizer information for this event
  virtual void SetDigitizer(const Digit &dig) { digitizer.push_back(dig); }

  /// Get CAEN digitizer information for this event
  virtual Digit &GetDigitizer() { return digitizer.at(0); };

  /// Check if the digitizer exists
  virtual bool DigitizerExists() const { return !digitizer.empty(); }

  // Prune digitizer information
  virtual void PruneDigitizer() { digitizer.resize(0); }

  /** Event Cleaning **/
  virtual uint64_t GetEventCleaningWord() const { return eventCleaningWord; }
  virtual void SetEventCleaningWord(uint64_t _eventCleaningWord) { eventCleaningWord = _eventCleaningWord; }
  virtual void SetEventCleaningBit(uint8_t bit_position, bool value = true) {
    if (bit_position >= std::numeric_limits<uint64_t>::digits) {
      warn << "Tried to set bit out of event cleaning bit mask range, ignoring." << newline;
      return;
    }
    eventCleaningWord = (eventCleaningWord & ~(1ULL << bit_position)) | (value << bit_position);
  }
  virtual bool GetEventCleaningBit(uint8_t bit_position) const {
    if (bit_position >= std::numeric_limits<uint64_t>::digits) {
      warn << "Tried to get bit out of event cleaning bit mask range, ignoring." << newline;
      return false;
    }
    return (eventCleaningWord >> bit_position) & 0x1;
  }

  ClassDef(EV, 5);

 protected:
  Int_t id;
  Double_t qTotal;
  Double_t calibratedTriggerTime;
  Double_t deltat;
  TTimeStamp utc;
  uint64_t trigger_word;
  std::map<Int_t, PMT> pmt;
  std::map<Int_t, DigitPMT> digitpmt;
  std::vector<LAPPD> lappd;
  std::vector<FitResult *> fitResults;
  std::vector<Classifier *> classifierResults;
  std::vector<Digit> digitizer;  ///< The digitizer information
  uint64_t eventCleaningWord = 0;
};

}  // namespace DS
}  // namespace RAT

#endif
