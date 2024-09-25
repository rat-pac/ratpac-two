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

  virtual void AddChannel(int lcn, int is_online, double offset) {
    lcns.push_back(lcn);
    online.push_back(is_online);
    cable_offset.push_back(offset);
    lcn_to_index[lcn] = lcns.size() - 1;
  }

  virtual bool GetOnlineByChannel(int lcn) const { return online.at(lcn_to_index.at(lcn)); }
  virtual bool GetOnlineByPMTID(int pmtid) const { return online.at(pmtid_to_index.at(pmtid)); }

  virtual double GetCableOffsetByChannel(int lcn) const { return cable_offset.at(lcn_to_index.at(lcn)); }
  virtual double GetCableOffsetByPMTID(int pmtid) const { return cable_offset.at(pmtid_to_index.at(pmtid)); }

  virtual void LinkPMT(int pmtid, int lcn) {
    if (lcn_to_index.find(lcn) == lcn_to_index.end()) {
      AddChannel(lcn, default_is_online, default_offset);
    }
    pmtid_to_index[pmtid] = lcn_to_index[lcn];
  }

  virtual void Load(const PMTInfo* pmtinfo, const std::string index = "") {
    for (int pmtid = 0; pmtid < pmtinfo->GetPMTCount(); pmtid++) {
      int lcn = pmtinfo->GetChannelNumber(pmtid);
      LinkPMT(pmtid, lcn);
    }
    // cable offset
    try {
      DBLinkPtr lCableOffset = DB::Get()->GetLink("cable_offset", index);
      std::vector<int> lcns = lCableOffset->GetIArray("channel_number");
      std::vector<double> values = lCableOffset->GetDArray("value");
      insert_values(lcns, values, &cable_offset);
    } catch (DBNotFoundError& e) {
      warn << "Cable offset table not found! Looking for table cable_offset[" << index << "]\n";
    }
    // cable offset
    try {
      DBLinkPtr lCableOffset = DB::Get()->GetLink("channel_online", index);
      std::vector<int> lcns = lCableOffset->GetIArray("channel_number");
      std::vector<int> values = lCableOffset->GetIArray("value");
      insert_values(lcns, values, &online);
    } catch (DBNotFoundError& e) {
      warn << "Cable offset table not found! Looking for table cable_offset[" << index << "]\n";
    }
    // read channel online
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

  static inline const double default_offset = 0.0;
  static inline const int default_is_online = 1;

  ClassDef(ChannelStatus, 1);

 protected:
  std::map<int, size_t> lcn_to_index;
  std::map<int, size_t> pmtid_to_index;
  std::vector<int> lcns;
  std::vector<int> online;
  std::vector<double> cable_offset;
};

}  // namespace DS
}  // namespace RAT

#endif
