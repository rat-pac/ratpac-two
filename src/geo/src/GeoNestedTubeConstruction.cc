#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Material.hh>
#include <G4SDManager.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <RAT/DB.hh>
#include <RAT/GeoFiberSensitiveDetector.hh>
#include <RAT/GeoNestedSolidArrayFactoryBase.hh>
#include <RAT/GeoNestedTubeConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <algorithm>

namespace RAT {

GeoNestedTubeConstruction::GeoNestedTubeConstruction(DBLinkPtr table, DBLinkPtr postable, G4LogicalVolume *mother,
                                                     int ID) {
  inner_phys = 0;
  core_phys = 0;

  log_tube = 0;

  myTable = table;

  // Setup NestedTube parameters
  fParams.outer_r = table->GetD("outer_r");
  fParams.inner_r = table->GetD("inner_r");
  fParams.core_r = table->GetD("core_r");

  // Materials
  fParams.outer = G4Material::GetMaterial(table->GetS("outer_material"));
  fParams.inner = G4Material::GetMaterial(table->GetS("inner_material"));
  fParams.core = G4Material::GetMaterial(table->GetS("core_material"));
  std::string core_surface_name = table->GetS("core_material");
  if (Materials::optical_surface.count(core_surface_name) == 0)
    Log::Die("GeoSolidFactory: Surface " + core_surface_name + " does not exist");
  fParams.inner_core = Materials::optical_surface[core_surface_name];
  fParams.Dz = postable->GetDArray("Dz")[ID];

  std::string tube_name = table->GetS("index");
  Log::Assert(fParams.outer_r > 0, "GeoNestedTubeConstruction: " + tube_name + " outer radius must be positive");
  Log::Assert(fParams.inner_r > 0, "GeoNestedTubeConstruction: " + tube_name + " inner radius must be positive");
  Log::Assert(fParams.core_r > 0, "GeoNestedTubeConstruction: " + tube_name + " core radius must be positive");
  Log::Assert(fParams.outer_r > fParams.inner_r,
              "GeoNestedTubeConstruction: " + tube_name + " outer radius is smaller than inner radius");
  Log::Assert(fParams.inner_r > fParams.core_r,
              "GeoNestedTubeConstruction: " + tube_name + " inner radius is smaller than core radius");
  Log::Assert(fParams.outer, "GeoNestedTubeConstruction: " + tube_name + " has an invalid outer material");
  Log::Assert(fParams.inner, "GeoNestedTubeConstruction: " + tube_name + " has an invalid inner material");
  Log::Assert(fParams.core, "GeoNestedTubeConstruction: " + tube_name + " has an invalid core material");
  Log::Assert(fParams.inner_core, "GeoNestedTubeConstruction: " + tube_name + " has an invalid core surface material");
}

G4LogicalVolume *GeoNestedTubeConstruction::BuildVolume(const std::string &prefix, int ID, DBLinkPtr table) {
  if (log_tube) {
    return log_tube;
  }

  // fibre outer
  G4Tubs *outer_solid = (G4Tubs *)BuildSolid(prefix + "_outer_solid");

  // fibre inner
  G4Tubs *inner_solid = new G4Tubs(prefix + "_inner_solid", 0.0, fParams.inner_r, fParams.Dz, 0.0, CLHEP::twopi);

  // fibre core
  G4Tubs *core_solid = new G4Tubs(prefix + "_core_solid", 0.0, fParams.core_r, fParams.Dz, 0.0, CLHEP::twopi);

  // ------------ Logical Volumes -------------
  G4LogicalVolume *outer_log, *inner_log, *core_log;

  outer_log = new G4LogicalVolume(outer_solid, fParams.outer, prefix + "_outer_logic");
  inner_log = new G4LogicalVolume(inner_solid, fParams.inner, prefix + "_inner_logic");
  core_log = new G4LogicalVolume(core_solid, fParams.core, prefix + "_core_logic");
  // set as sensitive detector if applicable
  try {
    std::string sensitive_detector = table->GetS("sensitive_detector");
    GeoFiberSensitiveDetector *sensitive = new GeoFiberSensitiveDetector(sensitive_detector + std::to_string(ID));
    G4SDManager *sdman = G4SDManager::GetSDMpointer();
    sdman->AddNewDetector(sensitive);
    core_log->SetSensitiveDetector(sensitive);
  } catch (DBNotFoundError &e) {
  }

  // ------------ Physical Volumes -------------
  G4ThreeVector noTranslation(0., 0., 0.);

  // Place the core solids in the inner solid to produce the physical volumes
  inner_phys = new G4PVPlacement(0,                       // no rotation
                                 noTranslation,           // place inner tube concentric to outer
                                 inner_log,               // the logical volume
                                 prefix + "_inner_phys",  // a name for this physical volume
                                 outer_log,               // the mother volume
                                 false,                   // no boolean ops
                                 0);                      // copy number

  core_phys = new G4PVPlacement(0,                                            // no rotation
                                noTranslation,                                // place inner tube concentric to outer
                                core_log,                                     // the logical volume
                                prefix + "_" + std::to_string(ID) + "_core",  // a name for this physical volume
                                inner_log,                                    // the mother volume
                                false,                                        // no boolean ops
                                0);                                           // copy number

  // ------------ Vis Attributes -------------
  G4VisAttributes *vis = new G4VisAttributes();
  G4VisAttributes *vis_inner = new G4VisAttributes();
  G4VisAttributes *vis_core = new G4VisAttributes();
  try {
    const std::vector<double> &color = myTable->GetDArray("color");
    if (color.size() == 3) {  // RGB
      vis->SetColour(G4Colour(color[0], color[1], color[2]));
      vis_inner->SetColour(G4Colour(color[2], color[0], color[1]));
      vis_core->SetColour(G4Colour(color[1], color[2], color[0]));
    } else if (color.size() == 4) {  // RGBA
      vis->SetColour(G4Colour(color[0], color[1], color[2], color[3]));
      vis_inner->SetColour(G4Colour(color[2], color[0], color[1], color[3]));
      vis_core->SetColour(G4Colour(color[1], color[2], color[0], color[3]));
    } else
      warn << "GeoNestedTubeConstruction error: " << myTable->GetName() << "[" << myTable->GetIndex()
           << "].color must have 3 or 4 components" << newline;
  } catch (DBNotFoundError &e) {
  };
  try {
    std::string drawstyle = myTable->GetS("drawstyle");
    if (drawstyle == "wireframe") {
      vis->SetForceWireframe(true);
      vis_inner->SetForceWireframe(true);
      vis_core->SetForceWireframe(true);
    } else if (drawstyle == "solid") {
      vis->SetForceSolid(true);
      vis_inner->SetForceSolid(true);
      vis_core->SetForceSolid(true);
    } else
      warn << "GeoNestedTubeConstruction error: " << myTable->GetName() << "[" << myTable->GetIndex()
           << "].drawstyle must be either \"wireframe\" or \"solid\".";
  } catch (DBNotFoundError &e) {
  };

  // Check for invisible flag last
  try {
    int invisible = myTable->GetI("invisible");
    if (invisible) {
      outer_log->SetVisAttributes(G4VisAttributes::GetInvisible());
      inner_log->SetVisAttributes(G4VisAttributes::GetInvisible());
      core_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    }
  } catch (DBNotFoundError &e) {
  };

  outer_log->SetVisAttributes(vis);
  inner_log->SetVisAttributes(vis_inner);
  core_log->SetVisAttributes(vis_core);

  log_tube = outer_log;

  return log_tube;
}

G4VSolid *GeoNestedTubeConstruction::BuildSolid(const std::string &name) {
  G4Tubs *outer = new G4Tubs(name, 0.0, fParams.outer_r, fParams.Dz, 0.0, CLHEP::twopi);
  return outer;
}

G4PVPlacement *GeoNestedTubeConstruction::PlaceNestedTube(G4RotationMatrix *tuberot, G4ThreeVector tubepos,
                                                          const std::string &name, G4LogicalVolume *logi_tube,
                                                          G4VPhysicalVolume *mother_phys, bool booleanSolid,
                                                          int copyNo) {
  G4PVPlacement *outer_phys = new G4PVPlacement(tuberot, tubepos, name, logi_tube, mother_phys, booleanSolid, copyNo);

  // core surface
  new G4LogicalBorderSurface(name + "_core_logsurf1", core_phys, inner_phys, fParams.inner_core);

  // build the inner surface
  // new G4LogicalBorderSurface(name + "_inner_logsurf1", outer_phys, inner_phys, fParams.outer_inner);

  return outer_phys;
}

}  // namespace RAT
