#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4VisAttributes.hh>
#include <RAT/DB.hh>
#include <RAT/DS/NestedTubeInfo.hh>
#include <RAT/GeoFactory.hh>
#include <RAT/GeoNestedSolidArrayFactoryBase.hh>
#include <RAT/GeoNestedTubeConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <vector>

namespace RAT {

DS::NestedTubeInfo GeoNestedSolidArrayFactoryBase::nestedtubeinfo;

G4VPhysicalVolume *GeoNestedSolidArrayFactoryBase::Construct(DBLinkPtr table) {
  std::string volume_name = table->GetIndex();
  std::string volume_name_one = volume_name + "one";
  std::string volume_name_two = volume_name + "two";
  std::string mother_name = table->GetS("mother");

  G4LogicalVolume *mother = FindMother(mother_name);
  if (mother == 0) {
    Log::Die("GeoNestedTube: Unable to find mother volume " + mother_name + " for " + volume_name);
  }
  // build logical volume of nested tube. Contains volumes for inner and core.
  // GeoNestedTubeConstruction *construction = new GeoNestedTubeConstruction(table, mother);
  // G4LogicalVolume *log_tube = construction->BuildVolume(volume_name);

  // Read Solid positions
  // TODO: The programmers guide gives an alternative for GetDArray that will be faster when you have large arrays ->
  // our arrays will be on order 6k so we should switch
  std::string pos_table_name = table->GetS("pos_table");
  DBLinkPtr lpos_table = DB::Get()->GetLink(pos_table_name);
  const std::vector<double> &pos_x = lpos_table->GetDArray("x");
  const std::vector<double> &pos_y = lpos_table->GetDArray("y");
  const std::vector<double> &pos_z = lpos_table->GetDArray("z");

  // read max number of solids to use
  int max_solids = pos_x.size();  // default to read all
  try {
    max_solids = table->GetI("max_num");
  } catch (DBNotFoundError &e) {
  }
  if (max_solids > (int)pos_x.size()) max_solids = pos_x.size();

  // read starting number of solids to use
  int start_solid_num = 0;  // default to read all
  try {
    start_solid_num = table->GetI("start_num");
  } catch (DBNotFoundError &e) {
  }
  if (start_solid_num < 0) start_solid_num = 0;

  // Try to see if a sub type has been specified
  int sub_type = -1;  // default to read all
  try {
    sub_type = table->GetI("sub_type");
  } catch (DBNotFoundError &e) {
  }

  std::vector<int> sub_type_array;
  for (int i = 0; i < max_solids; i++) sub_type_array.push_back(-1);
  if (sub_type > -1) {
    sub_type_array = lpos_table->GetIArray("sub_type");
  }

  // direction of individual solids.  Default is that +z is orientation pointing
  // direction optional, default is no rotation
  bool rot_manual = false;
  std::vector<double> rot_x, rot_y, rot_z;
  try {
    std::string rotate_str = table->GetS("rotate_solids");
    if (rotate_str == "manual") rot_manual = true;
  } catch (DBNotFoundError &e) {
  }
  if (rot_manual) {
    rot_x = lpos_table->GetDArray("rot_x");
    rot_y = lpos_table->GetDArray("rot_y");
    rot_z = lpos_table->GetDArray("rot_z");
  }

  // Orientation of Solids
  bool orient_manual = false;
  try {
    std::string orient_str = table->GetS("orientation");
    if (orient_str == "manual")
      orient_manual = true;
    else if (orient_str == "point")
      orient_manual = false;
    else
      Log::Die("GeoBuilder error: Unknown solid orientation " + orient_str);
  } catch (DBNotFoundError &e) {
  }

  std::vector<double> dir_x, dir_y, dir_z;
  std::vector<double> orient_point_array;
  G4ThreeVector orient_point;
  if (orient_manual) {
    dir_x = lpos_table->GetDArray("dir_x");
    dir_y = lpos_table->GetDArray("dir_y");
    dir_z = lpos_table->GetDArray("dir_z");
  } else {
    orient_point_array = table->GetDArray("orient_point");
    if (orient_point_array.size() != 3) Log::Die("GeoBuilder error: orient_point must have 3 values");
    orient_point.set(orient_point_array[0], orient_point_array[1], orient_point_array[2]);
  }

  // Optionally can rescale Solid radius from mother volume center for
  // case where Solids have spherical layout symmetry

  bool rescale_radius = false;
  double new_radius = 1.0;
  try {
    new_radius = table->GetD("rescale_radius");
    rescale_radius = true;
  } catch (DBNotFoundError &e) {
  }

  // get pointer to physical mother volume
  // ##outer_tank##
  G4VPhysicalVolume *phys_mother = GeoFactory::FindPhysMother(mother_name);

  // create physical volumes for each fibre by placing logiSolid in mother volume
  for (int solidID = start_solid_num; solidID < max_solids; solidID++) {
    if ((sub_type == -1) || (sub_type == sub_type_array[solidID])) {
      // construct
      GeoNestedTubeConstruction *construction = new GeoNestedTubeConstruction(table, lpos_table, mother, solidID);
      G4LogicalVolume *log_tube = construction->BuildVolume(volume_name, solidID, table);
      // name
      std::string tubename = volume_name + "_" + ::to_string(solidID);

      // position
      G4ThreeVector tubepos(pos_x[solidID], pos_y[solidID], pos_z[solidID]);

      // direction
      G4ThreeVector soliddir;
      if (orient_manual)
        soliddir.set(dir_x[solidID], dir_y[solidID], dir_z[solidID]);
      else
        soliddir = orient_point - tubepos;
      soliddir = soliddir.unit();

      // rescale
      if (rescale_radius) tubepos.setMag(new_radius);

      // rotation required to point in direction of soliddir
      double angle_y = (-1.0) * atan2(soliddir.x(), soliddir.z());
      double angle_x = atan2(soliddir.y(), sqrt(soliddir.x() * soliddir.x() + soliddir.z() * soliddir.z()));
      double angle_z = atan2(-1 * soliddir.y() * soliddir.z(), soliddir.x());

      G4RotationMatrix *tuberot = new G4RotationMatrix();

      tuberot->rotateY(angle_y);
      tuberot->rotateX(angle_x);
      tuberot->rotateZ(angle_z);

      if (rot_manual) {
        tuberot->rotateZ(rot_z[solidID] * CLHEP::deg);
        tuberot->rotateY(rot_y[solidID] * CLHEP::deg);
        tuberot->rotateX(rot_x[solidID] * CLHEP::deg);
      }
      // ****************************************************************
      // * Use the constructor that specifies the PHYSICAL mother, since
      // * each Solid occurs only once in one physical volume.  This saves
      // * the GeometryManager some work. -GHS.
      // ****************************************************************

      // Write the real fiber positions and directions.
      // This goes into the DS by way of Gsim
      double length = lpos_table->GetDArray("Dz")[solidID] * 2;
      double core_r = table->GetD("core_r");
      double inner_r = table->GetD("inner_r");
      double outer_r = table->GetD("outer_r");
      std::string core_material = table->GetS("core_material");
      std::string inner_material = table->GetS("inner_material");
      std::string outer_material = table->GetS("outer_material");
      nestedtubeinfo.AddNestedTube(tubepos, soliddir, length, core_r, inner_r, outer_r, core_material, inner_material,
                                   outer_material);

      // instance of physical volume for fibre inside mother volume
      construction->PlaceNestedTube(tuberot, tubepos, tubename, log_tube, phys_mother, false, solidID);

    }  // end loop over solidID
  }
  return 0;
}

}  // namespace RAT
