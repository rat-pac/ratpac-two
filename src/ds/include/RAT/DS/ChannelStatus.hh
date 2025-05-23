/**
 * @class DS::ChannelStatus
 * Data Structure: Hardware channel status information
 *
 * @author James Shen <jierans@sas.upenn.edu>
 * Information about hardware channels, including which channels are online,
 * cable delays, etc.
 *
 * Channel information is stored in terms of Logical Channel Numbers (LCNs).
 * Mapping to PMTs are stored in PMTINFO under the `channel_number` field.
 */

#ifndef __RAT_DS_ChannelStatus__
#define __RAT_DS_ChannelStatus__

#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/Log.hh>
#include <map>
#include <string>

namespace RAT {
namespace DS {
class ChannelStatus : public TObject {
 public:
  ChannelStatus() : TObject() {}
  virtual ~ChannelStatus() {}

  virtual void AddChannel(int lcn, int is_online, double offset, double chargescale, double pulsewidthscale) {
    lcns.push_back(lcn);
    online.push_back(is_online);
    cable_offset.push_back(offset);
    charge_scale.push_back(chargescale);
    pulse_width_scale.push_back(pulsewidthscale);
    lcn_to_index[lcn] = lcns.size() - 1;
  }

  virtual bool GetOnlineByChannel(int lcn) const { return online.at(lcn_to_index.at(lcn)); }
  virtual bool GetOnlineByPMTID(int pmtid) const { return online.at(pmtid_to_index.at(pmtid)); }

  virtual double GetCableOffsetByChannel(int lcn) const { return cable_offset.at(lcn_to_index.at(lcn)); }
  virtual double GetCableOffsetByPMTID(int pmtid) const { return cable_offset.at(pmtid_to_index.at(pmtid)); }

  virtual double GetChargeScaleByChannel(int lcn) const { return charge_scale.at(lcn_to_index.at(lcn)); }
  virtual double GetChargeScaleByPMTID(int pmtid) const { return charge_scale.at(pmtid_to_index.at(pmtid)); }

  virtual double GetPulseWidthScaleByChannel(int lcn) const { return pulse_width_scale.at(lcn_to_index.at(lcn)); }
  virtual double GetPulseWidthScaleByPMTID(int pmtid) const { return pulse_width_scale.at(pmtid_to_index.at(pmtid)); }

  virtual void LinkPMT(int pmtid, int lcn) {
    // create entry with default values if none are specified
    if (lcn_to_index.find(lcn) == lcn_to_index.end()) {
      AddChannel(lcn, default_is_online, default_offset, default_charge_scale, default_pulse_width_scale);
    }
    pmtid_to_index[pmtid] = lcn_to_index[lcn];
  }

  virtual void Load(const PMTInfo* pmtinfo, const std::string index = "") {
    DBLinkPtr lChannelStatusSelection = DB::Get()->GetLink("channel_status_selection", index);
    DBLinkPtr lCableOffset = get_dblink_for_status("cable_offset", lChannelStatusSelection);
    default_offset = lCableOffset->GetD("default_value");
    DBLinkPtr lChannelOnline = get_dblink_for_status("channel_online", lChannelStatusSelection);
    default_is_online = lChannelOnline->GetI("default_value");
    DBLinkPtr lChargeScale = get_dblink_for_status("charge_scale", lChannelStatusSelection);
    default_charge_scale = lChargeScale->GetD("default_value");
    DBLinkPtr lPulseWidthScale = get_dblink_for_status("pulse_width_scale", lChannelStatusSelection);
    default_pulse_width_scale = lPulseWidthScale->GetD("default_value");

    // Set all channel status entries with default values first.
    for (int pmtid = 0; pmtid < pmtinfo->GetPMTCount(); pmtid++) {
      int lcn = pmtinfo->GetChannelNumber(pmtid);
      LinkPMT(pmtid, lcn);
    }

    // override with per-channel values specified in tables.
    populate_status<double>(lCableOffset, &cable_offset);
    populate_status<double>(lChargeScale, &charge_scale);
    populate_status<double>(lPulseWidthScale, &pulse_width_scale);
    populate_status<int>(lChannelOnline, &online);
  }

  std::vector<int> get_lcns(DBLinkPtr& lTable) {
    std::vector<int> lcns;
    // I'm sorry for the nested try catch blocks
    try {
      lcns = lTable->GetIArray("channel_number");
    } catch (DBNotFoundError& e) {
      try {
        std::vector<int> lcn_range = lTable->GetIArray("channel_number_range");
        Log::Assert(lcn_range.size() == 2, "Expect a length-2 array for channel_number_range");
        Log::Assert(lcn_range.at(0) <= lcn_range.at(1), "begin element must be smaller or equal to end");
        for (int current_lcn = lcn_range.at(0); current_lcn <= lcn_range.at(1); current_lcn++) {
          lcns.push_back(current_lcn);
        }
      } catch (DBNotFoundError& e) {
        throw;  // upstream should handle this
      }
    }
    return lcns;
  }

  template <typename T>
  void insert_values(std::vector<int> lcns, std::vector<T> values, std::vector<T>* target) {
    Log::Assert(lcns.size() == values.size(), "LCN and value arrays are of different lengths!");
    for (size_t i = 0; i < lcns.size(); i++) {
      int current_lcn = lcns.at(i);
      if (lcn_to_index.find(current_lcn) == lcn_to_index.end()) {
        LinkPMT(-current_lcn, current_lcn);
      }
      double current_value = values.at(i);
      size_t insertion_index = lcn_to_index.at(current_lcn);
      target->at(insertion_index) = current_value;
    }
  }

  DBLinkPtr get_dblink_for_status(const std::string& status_name, DBLinkPtr channel_status_select) {
    std::string index;
    try {
      index = channel_status_select->GetS(status_name);
    } catch (DBNotFoundError) {
      index = channel_status_select->GetS("default");
    }
    info << "Using " << status_name << "[" << index << "] for channel status" << newline;
    return DB::Get()->GetLink(status_name, index);
  }

  template <typename T>
  void populate_status(DBLinkPtr lStatus, std::vector<T>* target) {
    try {
      std::vector<int> lcns = get_lcns(lStatus);
      std::vector<T> values = lStatus->Get<std::vector<T>>("value");
      insert_values(lcns, values, target);
    } catch (DBNotFoundError& e) {
      warn << lStatus->GetName() << "[" << lStatus->GetIndex() << "]"
           << " has no per-channel table. Using the default value. \n";
    }
  }
  ClassDef(ChannelStatus, 3);

 protected:
  std::map<int, size_t> lcn_to_index;
  std::map<int, size_t> pmtid_to_index;
  std::vector<int> lcns;
  std::vector<int> online;
  std::vector<double> cable_offset;
  std::vector<double> charge_scale;
  std::vector<double> pulse_width_scale;
  double default_offset;
  int default_is_online;
  std::string ChargeScaleIndex;
  double default_charge_scale;
  double default_pulse_width_scale;
};

}  // namespace DS
}  // namespace RAT

#endif
