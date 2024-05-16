#ifndef __RAT_PMTFactoryBase__
#define __RAT_PMTFactoryBase__

#include <RAT/DS/PMTInfo.hh>
#include <RAT/GeoFactory.hh>

namespace RAT {
class PMTFactoryBase : public GeoFactory {
 public:
  PMTFactoryBase(const std::string &name) : GeoFactory(name){};
  static const DS::PMTInfo &GetPMTInfo() { return pmtinfo; }

 protected:
  virtual G4VPhysicalVolume *ConstructPMTs(DBLinkPtr table, const std::vector<G4ThreeVector> &pmt_pos,
                                           const std::vector<G4ThreeVector> &pmt_dir, const std::vector<int> &pmt_type,
                                           const std::vector<int> &pmt_channel_number,
                                           const std::vector<double> &pmt_effi_corr,
                                           const std::vector<double> &individual_noise_rate,
                                           const std::vector<double> &individual_afterpulse_fraction);

  static DS::PMTInfo pmtinfo;  /// keeps track of all the PMTs built into the geometry
};
}  // namespace RAT

#endif
