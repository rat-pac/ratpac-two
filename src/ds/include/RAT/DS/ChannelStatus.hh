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

  virtual void AddChannel(int lcn, int is_online, double offset, double chargescale) {
    lcns.push_back(lcn);
    online.push_back(is_online);
    cable_offset.push_back(offset);
    charge_scale.push_back(chargescale);
    lcn_to_index[lcn] = lcns.size() - 1;
  }

  virtual bool GetOnlineByChannel(int lcn) const { return online.at(lcn_to_index.at(lcn)); }
  virtual bool GetOnlineByPMTID(int pmtid) const { return online.at(pmtid_to_index.at(pmtid)); }

  virtual double GetCableOffsetByChannel(int lcn) const { return cable_offset.at(lcn_to_index.at(lcn)); }
  virtual double GetCableOffsetByPMTID(int pmtid) const { return cable_offset.at(pmtid_to_index.at(pmtid)); }

  virtual double GetChargeScaleByChannel(int lcn) const { return charge_scale.at(lcn_to_index.at(lcn)); }
  virtual double GetChargeScaleByPMTID(int pmtid) const { return charge_scale.at(pmtid_to_index.at(pmtid)); }

  virtual void LinkPMT(int pmtid, int lcn) {
    // create entry with default values if none are specified
    if (lcn_to_index.find(lcn) == lcn_to_index.end()) {
      AddChannel(lcn, default_is_online, default_offset, default_charge_scale);
    }
    pmtid_to_index[pmtid] = lcn_to_index[lcn];
  }

  virtual void Load(const PMTInfo* pmtinfo, const std::string index = "") {
    DBLinkPtr lCableOffset = DB::Get()->GetLink("cable_offset", index);
    default_offset = lCableOffset->GetD("default_value");
    DBLinkPtr lChannelOnline = DB::Get()->GetLink("channel_online", index);
    default_is_online = lChannelOnline->GetD("default_value");
    DBLinkPtr lChargeScaleIndex = DB::Get()->GetLink("charge_scale", "selected_charge_scale");
    ChargeScaleIndex = lChargeScaleIndex->GetS("selection");
    DBLinkPtr lChargeScale = DB::Get()->GetLink("charge_scale", ChargeScaleIndex);
    default_charge_scale = lChargeScale->GetD("default_value");
    for (int pmtid = 0; pmtid < pmtinfo->GetPMTCount(); pmtid++) {
      int lcn = pmtinfo->GetChannelNumber(pmtid);
      LinkPMT(pmtid, lcn);
    }
    // cable offset
    try {
      std::vector<int> lcns = get_lcns(lCableOffset);
      std::vector<double> values = lCableOffset->GetDArray("value");
      insert_values(lcns, values, &cable_offset);
    } catch (DBNotFoundError& e) {
      warn << "Cable offset table not found! Looking for table cable_offset[" << index << "]\n";
    }
    // charge scale
    try {
      std::vector<int> lcns = get_lcns(lChargeScale);
      std::vector<double> values = lChargeScale->GetDArray("value");
      insert_values(lcns, values, &charge_scale);
    } catch (DBNotFoundError& e) {
      warn << "Charge Scale table not found! Looking for table charge_scale[" << index << "]\n";
    }
    // dead channels
    try {
      std::vector<int> lcns = get_lcns(lChannelOnline);
      std::vector<int> values = lChannelOnline->GetIArray("value");
      insert_values(lcns, values, &online);
    } catch (DBNotFoundError& e) {
      warn << "Channel online table not found! Looking for table channel_online[" << index << "]\n";
    }
    // read channel online
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
        throw;  // upstream should hanndle this
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

  ClassDef(ChannelStatus, 2);

 protected:
  std::map<int, size_t> lcn_to_index;
  std::map<int, size_t> pmtid_to_index;
  std::vector<int> lcns;
  std::vector<int> online;
  std::vector<double> cable_offset;
  std::vector<double> charge_scale;
  double default_offset;
  int default_is_online;
  double default_charge_scale;
  std::string ChargeScaleIndex;
};

}  // namespace DS
}  // namespace RAT

#endif
