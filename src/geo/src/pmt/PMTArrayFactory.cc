#include <RAT/Log.hh>
#include <RAT/PMTArrayFactory.hh>
#include <RAT/PMTInfoParser.hh>
#include <vector>

namespace RAT {

G4VPhysicalVolume *PMTArrayFactory::Construct(DBLinkPtr table) {
  std::string volume_name = table->GetS("index");
  std::string mother_name = table->GetS("mother");

  info << "PMTArrayFactory: Constructing volume " + volume_name << newline;

  std::string pos_table_name = table->GetS("pos_table");
  DBLinkPtr lpos_table = DB::Get()->GetLink(pos_table_name);

  PMTInfoParser pmtinfo(lpos_table, mother_name);

  const std::vector<int> &pmtinfo_types = pmtinfo.GetTypes();
  const std::vector<G4ThreeVector> &pmtinfo_pos = pmtinfo.GetPMTLocations();
  const std::vector<G4ThreeVector> &pmtinfo_dir = pmtinfo.GetPMTDirections();
  const G4ThreeVector local_offset = pmtinfo.GetLocalOffset();
  const std::vector<double> &pmtinfo_effcorrs = pmtinfo.GetEfficiencyCorrections();
  const std::vector<double> &pmtinfo_noiserates = pmtinfo.GetPMTNoiseRates();
  const std::vector<double> &pmtinfo_afterpulse_fraction = pmtinfo.GetPMTAfterPulseFraction();
  const std::vector<int> &pmtinfo_channel_numbers = pmtinfo.GetChannelNumbers();

  int start_idx, end_idx;
  try {
    start_idx = table->GetI("start_idx");  // position in this array to start building pmts
  } catch (DBNotFoundError &e) {
    start_idx = 0;  // defaults to beginning
  }

  try {
    end_idx = table->GetI("end_idx");  // id of the last pmt to build in this array
  } catch (DBNotFoundError &e) {
    end_idx = pmtinfo_pos.size() - 1;  // defaults to whole array
  }
  info << "    start_idx: " << start_idx << "\n";
  info << "    end_idx: " << end_idx << "\n";

  // Flip direction of PMT relative to specified direction
  bool flip = false;
  try {
    flip = table->GetI("flip") != 0;
  } catch (DBNotFoundError &e) {
  }

  // Optionally can rescale PMT radius from mother volume center for
  // case where PMTs have spherical layout symmetry
  bool rescale_radius = false;
  double new_radius = 1.0;
  try {
    new_radius = table->GetD("rescale_radius");
    rescale_radius = true;
  } catch (DBNotFoundError &e) {
  }

  // Orientation of PMTs
  std::vector<double> dir_x, dir_y, dir_z;
  G4ThreeVector orient_point;
  bool orient_manual = false;
  try {
    std::string orient_str = table->GetS("orientation");
    if (orient_str == "manual") {
      orient_manual = true;
    } else if (orient_str == "point") {
      orient_manual = false;
    } else {
      Log::Die("PMTFactoryBase error: Unknown PMT orientation " + orient_str);
    }
  } catch (DBNotFoundError &e) {
  }

  if (!orient_manual) {
    const std::vector<double> &orient_point_array = table->GetDArray("orient_point");
    if (orient_point_array.size() != 3) {
      Log::Die("PMTFactoryBase error: orient_point must have 3 values");
    }
    orient_point.set(orient_point_array[0], orient_point_array[1], orient_point_array[2]);
  }

  std::vector<G4ThreeVector> pos(end_idx - start_idx + 1), dir(end_idx - start_idx + 1);
  std::vector<int> ptypes(end_idx - start_idx + 1);
  std::vector<int> channels(end_idx - start_idx + 1);
  for (int idx = start_idx, i = 0; idx <= end_idx; idx++, i++) {
    pos[i] = pmtinfo_pos[idx];
    ptypes[i] = pmtinfo_types[idx];
    channels[i] = pmtinfo_channel_numbers[idx];
    if (rescale_radius) {
      pos[i].setMag(new_radius);
    }
    pos[i] -= local_offset;

    if (orient_manual) {
      dir[i] = pmtinfo_dir[idx];
    } else {
      dir[i] = orient_point - pmtinfo_pos[idx];
    }
    dir[i] = dir[i].unit();

    if (flip) {
      dir[i] = -dir[i];
    }
  }

  return ConstructPMTs(table, pos, dir, ptypes, channels, pmtinfo_effcorrs, pmtinfo_noiserates,
                       pmtinfo_afterpulse_fraction);
}

}  // namespace RAT
