/** @class FitterInputHandler
 * Interface for handling the input stream of a standard fitter.
 *
 * @author James Shen <jierans@sas.upenn.edu>
 *
 * By default, the handler gets configured to one stream of input. This "steram"
 * can either be the DS::PMTs, DS::DigitPMTs, waveform analysis results, etc.
 *
 * */

#ifndef __RAT_FitterInputHandler__
#define __RAT_FitterInputHandler__

#include <RAT/DB.hh>
#include <RAT/DS/EV.hh>
#include <RAT/Log.hh>
#include <algorithm>
#include <string>

#include "RAT/DS/DigitPMT.hh"
#include "RAT/DS/PMT.hh"

namespace RAT {

class FitterInputHandler {
 public:
  enum class Mode {
    kPMT = 0,
    kDigitPMT = 1,
    kWaveformAnalysis = 2,
  };

  Mode mode;
  std::string wfm_ana_name;

  /**
   * Default constructor. Configures the input based on the FIT_COMMON entry.
   * */
  FitterInputHandler() : FitterInputHandler(""){};
  FitterInputHandler(const std::string& index) { Configure(index); };

  /**
   * Configures the class based on FIT_COMMON[index]
   * @param index  ratdb index to configure the class with.
   * */
  void Configure(const std::string& index) {
    DBLinkPtr tbl = DB::Get()->GetLink("FIT_COMMON", index);
    mode = static_cast<Mode>(tbl->GetI("mode"));
    if (mode == Mode::kWaveformAnalysis) wfm_ana_name = tbl->GetS("waveform_analyzer");
  }

  /**
   * Register an event to the input handler, so that we know to return the hits from this event.
   * @param _ev  event to register.
   * */
  void RegisterEvent(DS::EV* _ev) {
    ev = _ev;
    hitPMTChannels.clear();
    // NOTE: class implementation assumes GetAllPMTIDs and GetAllDigitPMTIDs returns _sorted_ results.
    // This is true since ev->digitpmt and ev->pmt are both std::maps.
    hitPMTChannels = mode == Mode::kPMT ? ev->GetAllPMTIDs() : ev->GetAllDigitPMTIDs();
  }

  /**
   * @brief Get PMTIDs for all pmts in the event.
   * PMT will not be in the list if it never created a hit on DS::PMT or if the digitized waveform never crossed
   * threshold.
   *
   * @return vector of all PMTs in event.
   */
  const std::vector<Int_t>& GetAllHitPMTIDs() {
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    return hitPMTChannels;
  }

  /**
   * @brief Get number of hit channels in the event.
   *
   */
  size_t GetNHits() {
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    return hitPMTChannels.size();
  }

  /**
   * @brief Get the charge of a pmt.
   * In the case where a waveoform analyzer created multiple hits on the PMT (multi-PE), this method only returns
   * information about this first hit. To get information about all the hits, use getCharges.
   *
   * @param id PMT ID.
   * @return charge of the first hit.
   */
  double GetCharge(Int_t id) {
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    if (!std::binary_search(hitPMTChannels.begin(), hitPMTChannels.end(), id))
      Log::Die("FitterInputHandler: Trying to access a channel with no hit registered!");

    switch (mode) {
      case Mode::kPMT:
        return ev->GetOrCreatePMT(id)->GetCharge();
      case Mode::kDigitPMT:
        return ev->GetOrCreateDigitPMT(id)->GetDigitizedCharge();
      case Mode::kWaveformAnalysis: {
        DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(id);
        std::vector<std::string> fitterNames = digitpmt->GetFitterNames();
        if (std::find(fitterNames.begin(), fitterNames.end(), wfm_ana_name) == fitterNames.end()) {
          info << "FitResult not found for pmt id " << id << " " << wfm_ana_name << newline;
        }
        return digitpmt->GetOrCreateWaveformAnalysisResult(wfm_ana_name)->getCharge(0);
      }
      default:
        Log::Die("INVALID TYPE! Should never reach here.");
    }
  }

  /**
   * @brief Get the charge of all hits registered on a PMT.
   * To only get information about the first hit (assume SPE), see getCharge.
   *
   * @param id PMT ID.
   * @return vector of the charges registered on all hits on the PMT.
   */
  std::vector<double> GetCharges(Int_t id) {
    if (mode != Mode::kWaveformAnalysis) return std::vector<double>{GetCharge(id)};
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    if (!std::binary_search(hitPMTChannels.begin(), hitPMTChannels.end(), id))
      Log::Die("FitterInputHandler: Trying to access a channel with no hit registered!");
    DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(id);
    std::vector<std::string> fitterNames = digitpmt->GetFitterNames();
    if (std::find(fitterNames.begin(), fitterNames.end(), wfm_ana_name) == fitterNames.end()) {
      info << "FitResult not found for pmt id " << id << " " << wfm_ana_name << newline;
    }
    return digitpmt->GetOrCreateWaveformAnalysisResult(wfm_ana_name)->getCharges();
  }

  /**
   * @brief Get the time of a pmt hit.
   * In the case where a waveoform analyzer created multiple hits on the PMT (multi-PE), this method only returns
   * information about this first hit. To get information about all the hits, use getTimes.
   * @param id PMT ID.
   *
   * @return time of the first hit.
   */
  double GetTime(Int_t id) {
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    if (!std::binary_search(hitPMTChannels.begin(), hitPMTChannels.end(), id))
      Log::Die("FitterInputHandler: Trying to access a channel with no hit registered!");

    switch (mode) {
      case Mode::kPMT:
        return ev->GetOrCreatePMT(id)->GetTime();
      case Mode::kDigitPMT:
        return ev->GetOrCreateDigitPMT(id)->GetDigitizedTime();
      case Mode::kWaveformAnalysis: {
        DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(id);
        std::vector<std::string> fitterNames = digitpmt->GetFitterNames();
        if (std::find(fitterNames.begin(), fitterNames.end(), wfm_ana_name) == fitterNames.end()) {
          info << "FitResult not found for pmt id " << id << " " << wfm_ana_name << newline;
        }
        return digitpmt->GetOrCreateWaveformAnalysisResult(wfm_ana_name)->getTime(0);
      }
      default:
        Log::Die("INVALID TYPE! Should never reach here.");
    }
  }

  /**
   * @brief Get the time of all hits registered on a PMT.
   * To only get information about the first hit (assume SPE), see getTime.
   *
   * @param id PMT ID.
   * @return vector of the times registered on all hits on the PMT.
   */
  std::vector<double> GetTimes(Int_t id) {
    if (mode != Mode::kWaveformAnalysis) return std::vector<double>{GetTime(id)};
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    if (!std::binary_search(hitPMTChannels.begin(), hitPMTChannels.end(), id))
      Log::Die("FitterInputHandler: Trying to access a channel with no hit registered!");
    DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(id);
    std::vector<std::string> fitterNames = digitpmt->GetFitterNames();
    if (std::find(fitterNames.begin(), fitterNames.end(), wfm_ana_name) == fitterNames.end()) {
      info << "FitResult not found for pmt id " << id << " " << wfm_ana_name << newline;
    }
    return digitpmt->GetOrCreateWaveformAnalysisResult(wfm_ana_name)->getTimes();
  }

  /**
   * @brief Return the (approximate) number of hits registered on a PMT.
   * Behavior is different depending on the mode.
   *  If mode is set to kPMT, always return 1 since no information about nhit is given.
   *  If mode is set to kDigitPMT, return the number of times that the waveform crosses threshold.
   *  If mode is set to kWaveformAnalysis, return the number of hits created by the analyzer.
   *
   * @param id PMT ID.
   */
  unsigned int GetNPEs(Int_t id) {
    if (!ev) Log::Die("FitterInputHandler: Trying to acccess event info without registering the event.");
    if (!std::binary_search(hitPMTChannels.begin(), hitPMTChannels.end(), id))
      Log::Die("FitterInputHandler: Trying to access a channel with no hit registered!");

    switch (mode) {
      case Mode::kPMT:
        return 1;  // no nhit information
      case Mode::kDigitPMT:
        return ev->GetOrCreateDigitPMT(id)->GetNCrossings();  // approximate
      case Mode::kWaveformAnalysis:
        DS::DigitPMT* digitpmt = ev->GetOrCreateDigitPMT(id);
        std::vector<std::string> fitterNames = digitpmt->GetFitterNames();
        if (std::find(fitterNames.begin(), fitterNames.end(), wfm_ana_name) == fitterNames.end()) {
          info << "FitResult not found for pmt id " << id << " " << wfm_ana_name << newline;
        }
        return digitpmt->GetOrCreateWaveformAnalysisResult(wfm_ana_name)->getNhits();
    }
  }

 protected:
  DS::EV* ev = nullptr;
  std::vector<Int_t> hitPMTChannels;
};

}  // namespace RAT

#endif
