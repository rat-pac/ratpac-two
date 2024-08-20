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
      warn << "PMT " << pmtid << " (LCN " << lcn << ") not found in channel_status, using defaults" << newline;
      AddChannel(lcn, true, 0.0);
    }
    pmtid_to_index[pmtid] = lcn_to_index[lcn];
  }
  virtual void Load(const PMTInfo* pmtinfo, const std::string index = "") {
    try {
      info << "Using channel status table with index: " << index << newline;
      DBLinkPtr lChStatus = DB::Get()->GetLink("channel_status", index);
      std::vector<int> lcns = lChStatus->GetIArray("channel_number");
      std::vector<int> onlines = lChStatus->GetIArray("online");
      std::vector<double> offsets = lChStatus->GetDArray("offset");
      for (size_t idx = 0; idx < lcns.size(); idx++) {
        AddChannel(lcns[idx], onlines[idx], offsets[idx]);
      }
    } catch (DBNotFoundError& e) {
      warn << "Channel Status table Not found!" << newline;
    }
    for (int pmtid = 0; pmtid < pmtinfo->GetPMTCount(); pmtid++) {
      int lcn = pmtinfo->GetChannelNumber(pmtid);
      LinkPMT(pmtid, lcn);
    }
  }

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
