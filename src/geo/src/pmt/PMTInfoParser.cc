#include <G4Material.hh>
#include <G4SDManager.hh>
#include <RAT/GeoFactory.hh>
#include <RAT/Materials.hh>
#include <RAT/PMTInfoParser.hh>
#include <string>

using namespace std;

namespace RAT {

PMTInfoParser::PMTInfoParser(DBLinkPtr lpos_table, const std::string &mother_name) {
  // These positions are in global coordinates and are stored in global coordinates
  try {
    const vector<double> &pos_x = lpos_table->GetDArray("x");
    const vector<double> &pos_y = lpos_table->GetDArray("y");
    const vector<double> &pos_z = lpos_table->GetDArray("z");
    Log::Assert(pos_x.size() == pos_y.size() && pos_x.size() == pos_z.size(),
                "PMTInfoParser: PMTINFO arrays must be same length!");
    fPos.resize(pos_x.size());

    for (size_t i = 0; i < pos_x.size(); i++) {
      fPos[i].set(pos_x[i], pos_y[i], pos_z[i]);
    }
  } catch (DBNotFoundError &e) {
    Log::Die("PMTInfoParser: PMTINFO does not specify position information");
  }

  // Direction does not have to be specified here
  try {
    const vector<double> &dir_x = lpos_table->GetDArray("dir_x");
    const vector<double> &dir_y = lpos_table->GetDArray("dir_y");
    const vector<double> &dir_z = lpos_table->GetDArray("dir_z");
    Log::Assert(fPos.size() == dir_x.size() && fPos.size() == dir_y.size() && fPos.size() == dir_z.size(),
                "PMTInfoParser: PMTINFO arrays must be same length!");
    fDir.resize(fPos.size());

    for (size_t i = 0; i < fPos.size(); i++) {
      fDir[i].set(dir_x[i], dir_y[i], dir_z[i]);
      fDir[i] = fDir[i].unit();
    }
  } catch (DBNotFoundError &e) {
    warn << "PMTInfoParser: PMTINFO does not specify direction information" << newline;
    fDir.resize(0);
  }

  try {
    fChannelNumber = lpos_table->GetIArray("channel_number");
    Log::Assert(fPos.size() == fChannelNumber.size(), "PMTInfoParser: PMTInfo arrays must be same length!");
  } catch (DBNotFoundError &e) {
    fChannelNumber.resize(fPos.size());
    fill(fChannelNumber.begin(), fChannelNumber.end(), -1);
  }

  // Logical type of PMT (e.g. normal, veto, etc)
  try {
    fType = lpos_table->GetIArray("type");  // functional type (e.g. inner, veto, etc. - arbitrary integers)
    Log::Assert(fPos.size() == fType.size(), "PMTInfoParser: PMTINFO arrays must be same length!");
  } catch (DBNotFoundError &e) {
    fType.resize(fPos.size());
    fill(fType.begin(), fType.end(), -1);  // defaults to type -1 if unspecified
  }

  // Individual PMT efficiency correction
  try {
    fEfficiencyCorrection = lpos_table->GetDArray("efficiency_corr");  // individual PMT efficiency corrections
    Log::Assert(fPos.size() == fEfficiencyCorrection.size(), "PMTInfoParser: PMTINFO arrays must be same length!");
  } catch (DBNotFoundError &e) {
    fEfficiencyCorrection.resize(fPos.size());
    fill(fEfficiencyCorrection.begin(), fEfficiencyCorrection.end(), 1.0);  // defaults to 1.0
  }

  try {
    fNoiseRate = lpos_table->GetDArray("noise_rate");  // noise rate for individual pmts
    Log::Assert(fPos.size() == fNoiseRate.size(), "PMTInfoParser: PMTINFO arrays must be same length!");
  } catch (DBNotFoundError &e) {
    fNoiseRate.resize(fPos.size());
    fill(fNoiseRate.begin(), fNoiseRate.end(), 0.0);  // defaults to 0.0 (no noise)
  }

  try {
    fAfterPulseFraction =
        lpos_table->GetDArray("afterpulse_fraction");  // fraction of hits with afterpulse for individual pmts
    Log::Assert(fPos.size() == fAfterPulseFraction.size(), "PMTInfoParser: PMTINFO arrays must be same length!");
  } catch (DBNotFoundError &e) {
    fAfterPulseFraction.resize(fPos.size());
    fill(fAfterPulseFraction.begin(), fAfterPulseFraction.end(), 0.0);  // defaults to 0.0 (no afterpulsing)
  }

  // Find mother volume
  G4LogicalVolume *log_mother = GeoFactory::FindMother(mother_name);
  if (log_mother == 0) {
    Log::Die("PMTParser: PMT mother logical volume " + mother_name + " not found");
  }
  G4VPhysicalVolume *phys_mother = GeoFactory::FindPhysMother(mother_name);
  if (phys_mother == 0) {
    Log::Die("PMTParser: PMT mother physical volume " + mother_name + " not found");
  }
  fLocalOffset = ComputeLocalOffset(mother_name);
}

G4ThreeVector PMTInfoParser::ComputeLocalOffset(const std::string &mother_name) {
  // PMTINFO is always in global coordinates - so calculate the local offset first
  G4ThreeVector offset(0.0, 0.0, 0.0);
  for (string parent_name = mother_name; parent_name != "";) {
    G4VPhysicalVolume *parent_phys = GeoFactory::FindPhysMother(parent_name);
    offset -= parent_phys->GetFrameTranslation();
    DBLinkPtr parent_table = DB::Get()->GetLink("GEO", parent_name);
    parent_name = parent_table->GetS("mother");
  }
  return offset;
}

G4RotationMatrix PMTInfoParser::GetPMTRotation(int i) const {
  G4RotationMatrix rot;
  const G4ThreeVector &dir = fDir[i];

  double angle_y = (-1.0) * atan2(dir.x(), dir.z());
  double angle_x = atan2(dir.y(), sqrt(dir.x() * dir.x() + dir.z() * dir.z()));

  rot.rotateY(angle_y);
  rot.rotateX(angle_x);

  return rot;
}

}  // namespace RAT
