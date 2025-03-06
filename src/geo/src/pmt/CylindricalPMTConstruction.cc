#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Region.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <RAT/CylindricalPMTConstruction.hh>
#include <RAT/GLG4PMTOpticalModel.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <algorithm>

using namespace std;

namespace RAT {

CylindricalPMTConstruction::CylindricalPMTConstruction(DBLinkPtr table, G4LogicalVolume *mother)
    : PMTConstruction("cylindrical") {
  glass_phys = 0;
  vacuum_phys = 0;
  log_pmt = 0;

  // Setup PMT parameters
  fParams.pmtRadius = table->GetD("pmt_radius");
  fParams.pmtHeight = table->GetD("pmt_height");
  fParams.photocathodeRadius = table->GetD("photocathode_radius");
  fParams.caseThickness = table->GetD("case_thickness");
  fParams.glassThickness = table->GetD("glass_thickness");

  // Materials
  fParams.outerCase = G4Material::GetMaterial(table->GetS("case_material"));
  fParams.glass = G4Material::GetMaterial(table->GetS("glass_material"));
  fParams.vacuum = G4Material::GetMaterial(table->GetS("pmt_vacuum_material"));
  string pc_surface_name = table->GetS("photocathode_surface");
  fParams.photocathode = Materials::optical_surface[pc_surface_name];

  if (fParams.photocathode == 0)
    Log::Die("PMTFactoryBase error: Photocathode surface \"" + pc_surface_name + "\" not found");

  // Set new overall correction if requested (not included in individual)
  try {
    float efficiency_correction = table->GetD("efficiency_correction");
    fParams.efficiencyCorrection = efficiency_correction;
  } catch (DBNotFoundError &e) {
  }

  string pmt_model = table->GetS("index");
  Log::Assert(fParams.pmtRadius > 0, "CylindricalPMTConstruction: " + pmt_model + " radius must be postive");
  Log::Assert(fParams.pmtHeight > 0, "CylindricalPMTConstruction: " + pmt_model + " height must be postive");
  Log::Assert(fParams.photocathodeRadius > 0,
              "CylindricalPMTConstruction: " + pmt_model + " photocathode radius must be postive");
  Log::Assert(fParams.caseThickness > 0,
              "CylindricalPMTConstruction: " + pmt_model + " case thickness must be postive");
  Log::Assert(fParams.glassThickness > 0,
              "CylindricalPMTConstruction: " + pmt_model + " glass thickness must be postive");
  Log::Assert(fParams.pmtRadius > fParams.caseThickness + fParams.photocathodeRadius,
              "CylindricalPMTConstruction: " + pmt_model + " radius is too small");
  Log::Assert(fParams.outerCase, "CylindricalPMTConstruction: " + pmt_model + " has an invalid case material");
  Log::Assert(fParams.glass, "CylindricalPMTConstruction: " + pmt_model + " has an invalid glass material");
  Log::Assert(fParams.vacuum, "CylindricalPMTConstruction: " + pmt_model + " has an invalid vacuum material");
  Log::Assert(fParams.photocathode,
              "CylindricalPMTConstruction: " + pmt_model + " has an invalid photocathode material");
}

G4LogicalVolume *CylindricalPMTConstruction::BuildVolume(const std::string &prefix) {
  if (log_pmt) return log_pmt;

  // Case envelope body
  G4Tubs *body_solid = (G4Tubs *)BuildSolid(prefix + "_body_solid");

  // Glass body
  G4Tubs *glass_solid = new G4Tubs(prefix + "_glass_solid", 0., fParams.pmtRadius - fParams.caseThickness,
                                   fParams.pmtHeight - fParams.caseThickness, 0., CLHEP::twopi);

  // Construct inners
  const double vacuumHeight = fParams.pmtHeight - fParams.caseThickness - fParams.glassThickness;
  G4Tubs *vacuum_solid =
      new G4Tubs(prefix + "_vacuum_solid", 0., fParams.photocathodeRadius, vacuumHeight, 0., CLHEP::twopi);

  // ------------ Logical Volumes -------------
  G4LogicalVolume *body_log, *glass_log, *vacuum_log;

  body_log = new G4LogicalVolume(body_solid, fParams.outerCase, prefix + "_body_logic");
  glass_log = new G4LogicalVolume(glass_solid, fParams.glass, prefix + "_glass_logic");
  vacuum_log = new G4LogicalVolume(vacuum_solid, fParams.vacuum, prefix + "_vacuum_logic");

  // ------------ Physical Volumes -------------
  G4ThreeVector noTranslation(0., 0., 0.);

  // Place the inner solids in the glass solid to produce the physical volumes
  glass_phys = new G4PVPlacement(0,                                               // no rotation
                                 G4ThreeVector(0.0, 0.0, fParams.caseThickness),  // place glass surface at case surface
                                 glass_log,                                       // the logical volume
                                 prefix + "_glass_phys",                          // a name for this physical volume
                                 body_log,                                        // the mother volume
                                 false,                                           // no boolean ops
                                 0);                                              // copy number

  vacuum_phys = new G4PVPlacement(0,                        // no rotation
                                  noTranslation,            // must share the same origin than the mother volume
                                                            // if we want the PMT optical model working properly
                                  vacuum_log,               // the logical volume
                                  prefix + "_vacuum_phys",  // a name for this physical volume
                                  glass_log,                // the mother volume
                                  false,                    // no boolean ops
                                  0);                       // copy number

  // ------------ FastSimulationModel -------------
  // 28-Jul-2006 WGS: Must define a G4Region for Fast Simulations
  // (change from Geant 4.7 to Geant 4.8).
  G4Region *body_region = new G4Region(prefix + "_GLG4_PMTOpticalRegion");
  body_region->AddRootLogicalVolume(glass_log);
  new GLG4PMTOpticalModel(prefix + "_optical_model", body_region, glass_log, fParams.photocathode,
                          fParams.efficiencyCorrection, 0.0, 0.0, 0.0 /*prepusling handled after absorption*/);

  // ------------ Vis Attributes -------------
  G4VisAttributes *visAtt;
  if (fParams.invisible) {
    visAtt = new G4VisAttributes(G4Color(0.0, 1.0, 1.0, 0.05));
  } else {
    // PMT case
    visAtt = new G4VisAttributes(G4Color(0.5, 0.0, 0.0, 0.5));
    body_log->SetVisAttributes(visAtt);
    // PMT glass
    visAtt = new G4VisAttributes(G4Color(0.0, 1.0, 1.0, 0.05));
    glass_log->SetVisAttributes(visAtt);
    // (surface of) interior vacuum is clear orangish gray on top (PC),
    // silvery blue on bottom (mirror)
    visAtt = new G4VisAttributes(G4Color(0.7, 0.5, 0.3, 0.27));
    vacuum_log->SetVisAttributes(visAtt);
  }

  log_pmt = body_log;

  return log_pmt;
}

G4VSolid *CylindricalPMTConstruction::BuildSolid(const string &name) {
  G4Tubs *body = new G4Tubs(name, 0., fParams.pmtRadius, fParams.pmtHeight, 0., CLHEP::twopi);
  return body;
}

G4PVPlacement *CylindricalPMTConstruction::PlacePMT(G4RotationMatrix *pmtrot, G4ThreeVector pmtpos,
                                                    const std::string &name, G4LogicalVolume *logi_pmt,
                                                    G4VPhysicalVolume *mother_phys, bool booleanSolid, int copyNo) {
  G4PVPlacement *body_phys = new G4PVPlacement(pmtrot, pmtpos, name, logi_pmt, mother_phys, booleanSolid, copyNo);

  // photocathode surface
  new G4LogicalBorderSurface(name + "_photocathode_logsurf1", vacuum_phys, glass_phys, fParams.photocathode);

  return body_phys;
}

}  // namespace RAT
