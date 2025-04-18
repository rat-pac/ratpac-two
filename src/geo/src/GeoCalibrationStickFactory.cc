#include <CLHEP/Units/SystemOfUnits.h>
#include <math.h>

#include <G4Colour.hh>
#include <G4LogicalBorderSurface.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4Orb.hh>
#include <G4PVPlacement.hh>
#include <G4SubtractionSolid.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <G4UnitsTable.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VisAttributes.hh>
#include <RAT/DB.hh>
#include <RAT/GeoCalibrationStickFactory.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <string>
#include <vector>

namespace RAT {

G4VPhysicalVolume *GeoCalibrationStickFactory::Construct(DBLinkPtr table) {
  info << "Building Calibration Stick" << newline;
  const std::string volumeName = table->GetIndex();
  const std::string motherName = table->GetS("mother");

  // Table properties
  const double innerDiameter = table->GetD("inner_diameter");
  const double outerDiameter = table->GetD("outer_diameter");
  const double bottomThickness = table->GetD("bottom_thickness");
  const double sourceThickness = table->GetD("source_thickness");
  const double stickLength = table->GetD("stick_length");
  const double sourcePosition = table->GetD("source_position");
  const std::vector<double> positionArray = table->GetDArray("position");

  G4ThreeVector offset(positionArray[0], positionArray[1], positionArray[2]);

  // Main outer tube
  G4Material *stickMaterial = G4Material::GetMaterial(table->GetS("stick_material"));
  // const std::vector<double> &sourceColor = table->GetDArray("source_vis_color");
  G4Tubs *stickSolid = new G4Tubs("CalibrationStick_StickSolid", innerDiameter / 2.0, outerDiameter / 2.0,
                                  stickLength / 2.0, 0, CLHEP::twopi);
  G4LogicalVolume *stickLog = new G4LogicalVolume(stickSolid, stickMaterial, "CalibrationStick_Stick");

  // Place endcap
  G4Material *bottomMaterial = G4Material::GetMaterial(table->GetS("bottom_material"));
  G4Tubs *bottomSolid =
      new G4Tubs("CalibrationStick_BottomSolid", 0, outerDiameter / 2.0, bottomThickness / 2.0, 0, CLHEP::twopi);
  G4LogicalVolume *bottomLog = new G4LogicalVolume(bottomSolid, bottomMaterial, "CalibrationStick_Bottom");
  // Source
  G4Material *sourceMaterial = G4Material::GetMaterial(table->GetS("source_material"));
  G4Tubs *sourceSolid =
      new G4Tubs("CalibrationStick_SourceSolid", 0, innerDiameter / 2.0, sourceThickness / 2.0, 0, CLHEP::twopi);
  G4LogicalVolume *sourceLog = new G4LogicalVolume(sourceSolid, sourceMaterial, "CalibrationStick_Source");
  // Tube is filled with air
  G4Material *gasMaterial = G4Material::GetMaterial(table->GetS("gas_material"));
  G4Tubs *gasSolid =
      new G4Tubs("CalibrationStick_GasSolid", 0, innerDiameter / 2.0, stickLength / 2.0, 0, CLHEP::twopi);
  G4LogicalVolume *gasLog = new G4LogicalVolume(gasSolid, gasMaterial, "CalibrationStick_Gas");

  // Surface properties: Shiny steel?
  // G4OpticalSurface *paintSurface = GetSurface(table->GetS("paint_surface"));

  // Put stuff in the mother volume
  G4VPhysicalVolume *motherPhys = FindPhysMother(motherName);

  G4RotationMatrix *rotation = motherPhys->GetObjectRotation();
  G4ThreeVector position = motherPhys->GetObjectTranslation();
  // second is rotation, first is position
  position = position + G4ThreeVector(0.0, 0.0, stickLength / 2.0 + bottomThickness) + offset;

  new G4PVPlacement(rotation, position, "CalibrationStick_Stick", stickLog, motherPhys, false, 0);
  new G4PVPlacement(rotation, position - G4ThreeVector(0, 0, (stickLength + bottomThickness) / 2.0),
                    "CalibrationStick_Bottom", bottomLog, motherPhys, false, 0);
  new G4PVPlacement(rotation, position, "CalibrationStick_Gas", gasLog, motherPhys, false, 0);
  new G4PVPlacement(rotation, position - G4ThreeVector(0, 0, (stickLength / 2.0) - sourcePosition),
                    "CalibrationStick_Source", sourceLog, motherPhys, false, 0);

  // Set visuals
  SetVis(stickLog, table->GetDArray("stick_color"));
  SetVis(bottomLog, table->GetDArray("bottom_color"));
  SetVis(sourceLog, table->GetDArray("source_color"));

  return NULL;  // This function should probably be void, fixme
}

// G4OpticalSurface* GeoCherenkovSourceFactory::GetSurface(std::string surface_name)
// {
//     if (Materials::optical_surface.count(surface_name) == 0)
//         Log::Die("error: surface "+ surface_name + " does not exist");
//     return Materials::optical_surface[surface_name];
// }

void GeoCalibrationStickFactory::SetVis(G4LogicalVolume *volume, std::vector<double> color) {
  G4VisAttributes *att = GetColor(color);
  att->SetForceSolid(true);
  volume->SetVisAttributes(att);
}

// This really should be available to all factories
G4VisAttributes *GeoCalibrationStickFactory::GetColor(std::vector<double> color) {
  if (color.size() == 4) {
    return new G4VisAttributes(G4Color(color[0], color[1], color[2], color[3]));
  } else if (color.size() == 3) {
    return new G4VisAttributes(G4Color(color[0], color[1], color[2]));
  } else {
    return new G4VisAttributes(G4Color());
  }
}

}  // namespace RAT
