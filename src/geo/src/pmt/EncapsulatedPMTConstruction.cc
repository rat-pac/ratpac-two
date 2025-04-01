// Adam T: A encapsulated pmt based off the Toroidal pmt construction model for
// button (some of the offesets will need adjusted for different pmt this will
// work for the r7081pe model)

#include <CLHEP/Units/PhysicalConstants.h>

#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4Region.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <RAT/EncapsulatedPMTConstruction.hh>
#include <RAT/GLG4PMTOpticalModel.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <algorithm>

#include <G4Box.hh>
#include <G4GenericPolycone.hh>
#include <G4Paraboloid.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>

namespace RAT {

EncapsulatedPMTConstruction::EncapsulatedPMTConstruction(
    DBLinkPtr table, G4LogicalVolume *mother)
    : PMTConstruction("Encapsulated") {
  body_phys = 0;
  inner1_phys = 0;
  inner2_phys = 0;
  central_gap_phys = 0;
  dynode_phys = 0;
  encapsulation_phys = 0;
  in_encapsulation_phys = 0;
  front_encapsulation_phys = 0;
  rear_encapsulation_phys = 0;
  front_metal_encapsulaion_flange_phys = 0;
  rear_metal_encapsulation_flange_phys = 0;
  acrylic_flange_encapsulaion_phys = 0;
  silica_bag_encapsulation_phys = 0;
  cable_encapsulation_phys = 0;
  optical_gel_encapsulation_phys = 0;

  log_pmt = 0;

  // Setup PMT parameters
  fParams.faceGap = 0.1 * CLHEP::mm;
  fParams.zEdge = table->GetDArray("z_edge");
  fParams.rhoEdge = table->GetDArray("rho_edge");
  fParams.zOrigin = table->GetDArray("z_origin");
  fParams.dynodeRadius = table->GetD("dynode_radius");
  fParams.dynodeTop = table->GetD("dynode_top");
  fParams.wallThickness = table->GetD("wall_thickness");
  // MFB
  fParams.photocathode_MINrho = 0.0;
  try {
    fParams.photocathode_MINrho = table->GetD("photocathode_MINrho");
  } catch (DBNotFoundError &e) {
  };
  fParams.photocathode_MAXrho = 0.0;
  try {
    fParams.photocathode_MAXrho = table->GetD("photocathode_MAXrho");
  } catch (DBNotFoundError &e) {
  };

  // Materials
  fParams.exterior = mother->GetMaterial();
  fParams.glass = G4Material::GetMaterial(table->GetS("glass_material"));
  fParams.dynode = G4Material::GetMaterial(table->GetS("dynode_material"));
  fParams.vacuum = G4Material::GetMaterial(table->GetS("pmt_vacuum_material"));
  std::string pc_surface_name = table->GetS("photocathode_surface");
  fParams.photocathode = Materials::optical_surface[pc_surface_name];
  std::string mirror_surface_name = table->GetS("mirror_surface");
  fParams.mirror = Materials::optical_surface[mirror_surface_name];
  fParams.dynode_surface =
      Materials::optical_surface[table->GetS("dynode_surface")];

  // Encapsulation
  fParams.in_encapsulation_material =
      G4Material::GetMaterial(table->GetS("inside_encapsulation_material"));
  fParams.front_encapsulation_material =
      G4Material::GetMaterial(table->GetS("front_encapsulation_material"));
  fParams.rear_encapsulation_material =
      G4Material::GetMaterial(table->GetS("rear_encapsulation_material"));
  fParams.metal_flange_material =
      G4Material::GetMaterial(table->GetS("metal_flange_material"));
  fParams.acrylic_flange_material =
      G4Material::GetMaterial(table->GetS("acrylic_flange_material"));
  fParams.silica_bag_material =
      G4Material::GetMaterial(table->GetS("silica_bag_material"));
  fParams.cable_material =
      G4Material::GetMaterial(table->GetS("cable_material"));
  fParams.optical_gel_material =
      G4Material::GetMaterial(table->GetS("optical_gel_material"));

  fParams.in_encapsulation_surface =
      Materials::optical_surface[table->GetS("inside_encapsulation_material")];
  fParams.front_encapsulation_surface =
      Materials::optical_surface[table->GetS("front_encapsulation_material")];
  fParams.rear_encapsulation_surface =
      Materials::optical_surface[table->GetS("rear_encapsulation_material")];
  fParams.metal_flange_surface =
      Materials::optical_surface[table->GetS("metal_flange_material")];
  fParams.acrylic_flange_surface =
      Materials::optical_surface[table->GetS("acrylic_flange_material")];
  fParams.silica_bag_surface =
      Materials::optical_surface[table->GetS("silica_bag_material")];
  fParams.cable_surface =
      Materials::optical_surface[table->GetS("cable_material")];
  fParams.optical_gel_surface =
      Materials::optical_surface[table->GetS("optical_gel_material")];



  /*

    fParams.front_encapsulation_material =
    G4Material::GetMaterial(table->GetS("front_encapsulation_material"));
    fParams.rear_encapsulation_material =
    G4Material::GetMaterial(table->GetS("rear_encapsulation_material"));
    fParams.metal_flange_material =
    G4Material::GetMaterial(table->GetS("metal_flange_material"));
    fParams.acrylic_flange_material =
    G4Material::GetMaterial(table->GetS("acrylic_flange_material"));
    fParams.silica_bag_material =
    G4Material::GetMaterial(table->GetS("silica_bag_material"));
    fParams.cable_material =
    G4Material::GetMaterial(table->GetS("cable_material"));

    fParams.front_encapsulation_surface =
    Materials::optical_surface[table->GetS("front_encapsultion_material")];
    fParams.rear_encapsulation_surface =
    Materials::optical_surface[table->GetS("rear_encapsulation_material")];
    fParams.metal_flange_surface =
    Materials::optical_surface[table->GetS("metal_flange_material")];
    fParams.acrylic_flange_surface =
    Materials::optical_surface[table->GetS("acrylic_flange_material")];
    fParams.silica_bag_surface =
    Materials::optical_surface[table->GetS("silica_bag_material")];
    fParams.cable_surface =
    Materials::optical_surface[table->GetS("cable_material")];
  */

  if (fParams.photocathode == 0) {
    Log::Die("EncapsulatedPMTConstruction error: Photocathode surface \"" +
             pc_surface_name + "\" not found");
  }

  // Set new overall correction if requested (not included in individual)
  try {
    double efficiency_correction = table->GetD("efficiency_correction");
    fParams.efficiencyCorrection = efficiency_correction;
  } catch (DBNotFoundError &e) {
  }

  // --------------- Start building PMT geometry ------------------

  // Setup for waveguide
  fWaveguideFactory = 0;
  try {
    std::string waveguide = table->GetS("waveguide");
    std::string waveguide_desc = table->GetS("waveguide_desc");
    std::string waveguide_table, waveguide_index;
    if (!DB::ParseTableName(waveguide_desc, waveguide_table, waveguide_index)) {
      Log::Die("EncapsulatedPMTConstruction: Waveguide descriptor name is not "
               "a valid RATDB table: " +
               waveguide_desc);
    }

    fWaveguideFactory = GlobalFactory<WaveguideFactory>::New(waveguide);
    fWaveguideFactory->SetTable(waveguide_table, waveguide_index);
    fParams.faceGap = fWaveguideFactory->GetZTop();
    fParams.minEnvelopeRadius = fWaveguideFactory->GetRadius();
  } catch (DBNotFoundError &e) {
  }

  // Build PMT
  fParams.useEnvelope = true; // disable the use of envelope volume for now

  assert(fParams.zEdge.size() == fParams.rhoEdge.size());
  assert(fParams.zEdge.size() == fParams.zOrigin.size() + 1);
  assert(fParams.exterior);
  assert(fParams.glass);
  assert(fParams.vacuum);
  assert(fParams.dynode);
  assert(fParams.photocathode);
  assert(fParams.mirror);
}

G4LogicalVolume *
EncapsulatedPMTConstruction::BuildVolume(const std::string &prefix) {
  if (log_pmt) {
    return log_pmt;
  }

  // envelope cylinder
  G4VSolid *envelope_solid = 0;
  if (fParams.useEnvelope) {
    envelope_solid = NewEnvelopeSolid(prefix + "_envelope_solid");
  }

  // glass body
  GLG4TorusStack *body_solid =
      (GLG4TorusStack *)BuildSolid(prefix + "_body_solid");

  // inner vacuum
  GLG4TorusStack *inner1_solid = new GLG4TorusStack(prefix + "_inner1_solid");
  GLG4TorusStack *inner2_solid = new GLG4TorusStack(prefix + "_inner2_solid");
  std::vector<double> innerZEdge, innerRhoEdge;
  G4double zLowestDynode;
  int equatorIndex;
  CalcInnerParams(body_solid, innerZEdge, innerRhoEdge, equatorIndex,
                  zLowestDynode);
  inner1_solid->SetAllParameters(equatorIndex, &innerZEdge[0], &innerRhoEdge[0],
                                 &fParams.zOrigin[0]);
  inner2_solid->SetAllParameters(
      fParams.zOrigin.size() - equatorIndex, &innerZEdge[equatorIndex],
      &innerRhoEdge[equatorIndex], &fParams.zOrigin[equatorIndex]);

  // dynode volume
  G4double hhDynode = (fParams.dynodeTop - zLowestDynode) / 2.0;
  G4Tubs *dynode_solid =
      new G4Tubs(prefix + "_dynode_solid", 0.0,
                 fParams.dynodeRadius, // solid cylinder (FIXME?)
                 hhDynode,             // half height of cylinder
                 0., CLHEP::twopi);    // cylinder complete in phi

  // tolerance gap between inner1 and inner2, needed to prevent overlap due to
  // floating point roundoff
  G4double hhgap =
      0.5e-3; // half the needed gap between the front and back of the PMT
  G4double toleranceGapRadius =
      innerRhoEdge[equatorIndex]; // the outer radius of the gap needs to be
  // equal to the inner radius of the PMT where
  // inner1 and inner2 join

  G4Tubs *central_gap_solid =
      new G4Tubs(prefix + "_central_gap_solid", 0.0,
                 toleranceGapRadius, // solid cylinder with same radius as PMT
                 hhgap,              // half height of cylinder
                 0., CLHEP::twopi);  // cylinder complete in phi

  // ------------ Logical Volumes -------------
  G4LogicalVolume *envelope_log = 0, *body_log, *inner1_log, *inner2_log,
                  *dynode_log, *central_gap_log;

  if (fParams.useEnvelope) {
    envelope_log = new G4LogicalVolume(envelope_solid, fParams.exterior,
                                       prefix + "_envelope_log");
  }

  body_log =
      new G4LogicalVolume(body_solid, fParams.glass, prefix + "_body_log");

  inner1_log =
      new G4LogicalVolume(inner1_solid, fParams.vacuum, prefix + "_inner1_log");

  inner2_log =
      new G4LogicalVolume(inner2_solid, fParams.vacuum, prefix + "_inner2_log");

  dynode_log =
      new G4LogicalVolume(dynode_solid, fParams.dynode, prefix + "_dynode_log");

  central_gap_log = new G4LogicalVolume(central_gap_solid, fParams.vacuum,
                                        prefix + "_central_gap_log");

  ///// Encapsulaiton
  // The default inner encapsulation diameter is: 40cm
  // front and back perpendicular to the PMT direction
  // rotation required to point in direction of pmtdir
  // double angle_y = (-1.0)*atan2(pmtdir.x(), pmtdir.z());
  // double angle_x = atan2(pmtdir.y(),
  // sqrt(pmtdir.x()*pmtdir.x()+pmtdir.z()*pmtdir.z()));

  // G4RotationMatrix* pmtrot = new G4RotationMatrix();
  // pmtrot->rotateY(angle_y);
  // pmtrot->rotateX(angle_x);..

  // fParams.front_encapsulation_material =
  // G4Material::GetMaterial("nakano_acrylic");
  // fParams.front_encapsulation_surface =
  // Materials::optical_surface[table->GetS("nakano_acrylic")];
  G4cout << "PMT encapsulation is added!! \n ";
  double enc_radius = 20.0;   // default radius
  double enc_thickness = 0.8; // 8mm encapsulation thickness

/*  G4VSolid *inner_encapsulation_solid = 0;
  inner_encapsulation_solid = optical_gel(prefix + "_inner_encapsulation_solid", body_solid);
  inner_encapsulation_log = new G4LogicalVolume(inner_encapsulation_solid,            // G4VSolid
                          fParams.front_encapsulation_material, // G4Material
                          prefix+"inner_encapsulation_log");

  G4VSolid *inner_encapsulation_solid =
      new G4Sphere("inner_encapsulation_solid",
                   (0.0)*CLHEP::cm,                   // rmin 20 cm
                   (enc_radius-0.001) * CLHEP::cm, // rmax: 20.8 cm
                   CLHEP::pi, CLHEP::twopi,            // phi
                   0., CLHEP::pi);                     // theta
  //G4VSolid *inner_encapsulation_solid = new G4SubtractionSolid("inner_encapsulation_solid", inner_encapsulation_solid1, body_solid, 0, G4ThreeVector(0.0, 0.0, 9.8*CLHEP::cm));

  G4LogicalVolume *inner_encapsulation_log =
      new G4LogicalVolume(inner_encapsulation_solid,            // G4VSolid
                          fParams.front_encapsulation_material, // G4Material
                          "inner_encapsulation_log");

/*  G4VSolid *front_encapsulation_solid1 =
      new G4Sphere("front_encapsulation_solid",
                   (enc_radius)*CLHEP::cm,                   // rmin 20 cm
                   (enc_radius+ enc_thickness) * CLHEP::cm, // rmax: 20.8 cm
                   0.5 * CLHEP::pi, CLHEP::twopi,            // phi
                   0., 0.5 * CLHEP::pi);                     // theta

    G4VSolid *front_encapsulation_solid = new G4UnionSolid("front_encapsulation_solid", inner_encapsulation_solid, front_encapsulation_solid1);
*/

  //front_encapsulation_solid = new G4SubtractionSolid("front_encapsulation_solid", front_encapsulation_solid, body_solid, 0, G4ThreeVector(0.0, 0.0, 9.8*CLHEP::cm));
  G4VSolid *optical_gel_encapsulation_solid = 0;//, in_encapsulation_solid = 0;
  optical_gel_encapsulation_solid = optical_gel_pmt_subtraction(prefix + "_optical_gel_encapsulation_solid", body_solid);
  
  G4LogicalVolume *optical_gel_encapsulation_log =
      new G4LogicalVolume(optical_gel_encapsulation_solid,            // G4VSolid
                          fParams.optical_gel_material, // 
                          "optical_gel_encapsulation_log");

  G4Sphere *in_encapsulation_solid =
       new G4Sphere("in_encapsulation_solid",
                    (0)*CLHEP::cm,                   // rmin 20 cm
                    (enc_radius) * CLHEP::cm, // rmax: 20.8 cm
                    0.0,
                    CLHEP::twopi, 0.0, CLHEP::twopi);              // theta

  G4LogicalVolume *in_encapsulation_log =
      new G4LogicalVolume(in_encapsulation_solid,            // G4VSolid
                          fParams.in_encapsulation_material, // G4Material
                          "in_encapsulation_log");


  /*G4Sphere *optical_gel_encapsulation_solid =
       new G4Sphere("optical_gel_encapsulation_solid",
                    (enc_radius+enc_thickness)*CLHEP::cm,                   // rmin 20 cm
                    5 * CLHEP::cm, // rmax: 20.8 cm
                    0.5 * CLHEP::pi, CLHEP::twopi,            // phi
                    0., 0.5 * CLHEP::pi); 
                    
    */                
                                        // theta, // G4Material




  G4Sphere *front_encapsulation_solid =
       new G4Sphere("front_encapsulation_solid",
                    (enc_radius)*CLHEP::cm,                   // rmin 20 cm
                    (enc_radius + enc_thickness) * CLHEP::cm, // rmax: 20.8 cm
                    0.5 * CLHEP::pi, CLHEP::twopi,            // phi
                    0., 0.5 * CLHEP::pi);                     // theta
  G4LogicalVolume *front_encapsulation_log =
      new G4LogicalVolume(front_encapsulation_solid,            // G4VSolid
                          fParams.front_encapsulation_material, // G4Material
                          "front_encapsulation_log");
  G4Sphere *rear_encapsulation_solid =
      new G4Sphere("rear_encapsulation_solid",
                   (enc_radius)*CLHEP::cm,                   // rmin 20 cm
                   (enc_radius + enc_thickness) * CLHEP::cm, // rmax: 20.8 cm
                   0.5 * CLHEP::pi, CLHEP::twopi,            // phi
                   0.5 * CLHEP::pi, 0.5 * CLHEP::pi);        // theta
  G4LogicalVolume *rear_encapsulation_log =
      new G4LogicalVolume(rear_encapsulation_solid,            // G4VSolid
                          fParams.rear_encapsulation_material, // G4Material
                          "rear_encapsulation_log");

  G4Tubs *front_metal_flange_solid = new G4Tubs("front_metal_flange_solid",
                                                21.0 * CLHEP::cm, // rmin
                                                25.3 * CLHEP::cm, // rmax
                                                0.4 * CLHEP::cm,  // size z
                                                0, CLHEP::twopi); // phi
  G4LogicalVolume *front_metal_flange_encapsulation_log =
      new G4LogicalVolume(front_metal_flange_solid,      // G4VSolid
                          fParams.metal_flange_material, // G4Material
                          "front_metal_flange_encapsulation_log");

  G4Tubs *rear_metal_flange_solid = new G4Tubs("rear_metal_flange_solid",
                                               21.0 * CLHEP::cm, // rmin
                                               25.3 * CLHEP::cm, // rmax
                                               0.4 * CLHEP::cm,  // size z
                                               0, CLHEP::twopi); // phi
  G4LogicalVolume *rear_metal_flange_encapsulation_log =
      new G4LogicalVolume(rear_metal_flange_solid,       // G4VSolid
                          fParams.metal_flange_material, // G4Material
                          "rear_metal_flange_encapsulation_log");

  G4Tubs *acrylic_flange_solid = new G4Tubs("acrylic_flange_solid",
                                            20.8 * CLHEP::cm, // rmin
                                            25.3 * CLHEP::cm, // rmax
                                            0.7 * CLHEP::cm,  // size z
                                            0, CLHEP::twopi); // phi
  G4LogicalVolume *acrylic_flange_encapsulation_log =
      new G4LogicalVolume(acrylic_flange_solid,            // G4VSolid
                          fParams.acrylic_flange_material, // G4Material
                          "acrylic_flange_encapsulation_log");
  G4Box *silica_bag_solid =
      new G4Box("silica_bag_solid", 18 * CLHEP::mm, 33 * CLHEP::mm,
                3 * CLHEP::mm); // zhalf
  G4LogicalVolume *silica_bag_encapsulation_log =
      new G4LogicalVolume(silica_bag_solid,            // G4VSolid
                          fParams.silica_bag_material, // G4Materil
                          "silica_bag_encapsulation_log");
  G4Tubs *cable_solid = new G4Tubs("cable_solid",
                                   0 * CLHEP::cm,    // rmin
                                   6.5 * CLHEP::mm,  // rmax
                                   4.5 * CLHEP::cm,  // size z
                                   0, CLHEP::twopi); // phi
  G4LogicalVolume *cable_encapsulation_log =
      new G4LogicalVolume(cable_solid,            // G4VSolid
                          fParams.cable_material, // G4Material
                          "cable_encapsulation_log");

  // ------------ Physical Volumes -------------
  G4ThreeVector noTranslation(0., 0., 0.);
  body_phys = 0;

  if (fParams.useEnvelope) {
    // place body in envelope 
    // this will set the pmt that triggers 
   /* body_phys = new G4PVPlacement(
        0,             // no rotation
        G4ThreeVector(0.0, 0.0,
                    9.8 * CLHEP::cm), // Bounding envelope already constructed to put equator
                       // at origin
        body_log,      // the logical volume
        prefix + "_body_phys", // a name for this physical volume
        envelope_log,          // the mother volume
        false,                 // no boolean ops
        0);                    // copy number*/
  }
  in_encapsulation_phys = new G4PVPlacement(
        0, // no rotation
        G4ThreeVector(0.0, 0. * CLHEP::cm,
                      0.0 * CLHEP::cm), // Bounding envelope already constructed
                                         // to put equator at origin
        in_encapsulation_log,         // the logical volume
        prefix + "in_encapsulation_phys",  // a name for this physical volume
        envelope_log,                        // the mother volume
        false,                           // no boolean ops
        0);
  body_phys = new G4PVPlacement( /// This subtracts the pmt from the in encapsulation volume 
        0,             // no rotation
        G4ThreeVector(0.0, 0.0,
                    9.8 * CLHEP::cm), // Bounding envelope already constructed to put equator
                       // at origin
        body_log,      // the logical volume
        prefix + "_body_phys", // a name for this physical volume
        in_encapsulation_log,          // the mother volume
        false,                 // no boolean ops
        0);                    // copy number 
   optical_gel_encapsulation_phys = new G4PVPlacement(
        0, // no rotation
        G4ThreeVector(0.0, 0. * CLHEP::cm,
                      0.0 * CLHEP::cm), // Bounding envelope already constructed
                                         // to put equator at origin
        optical_gel_encapsulation_log,         // the logical volume
        prefix + "in_encapsulation_phys",  // a name for this physical volume
        in_encapsulation_log,                        // the mother volume
        false,                           // no boolean ops
        0);

  front_encapsulation_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 0.0,
                    0.0 * CLHEP::cm), // Bounding envelope already constructed
                                       // to put equator at origin
      front_encapsulation_log,         // the logical volume
      prefix + "_encapsulation_phys",  // a name for this physical volume
      envelope_log,                        // the mother volume
      false,                           // no boolean ops
      0);                              // copy number
  rear_encapsulation_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 0.0,
                    0.0 * CLHEP::cm), // Bounding envelope already constructed
                                       // to put equator at origin
      rear_encapsulation_log,          // the logical volume
      prefix + "_encapsulation_phys",  // a name for this physical volume
      envelope_log,                        // the mother volume
      false,                           // no boolean ops
      0);                              // copy number
  front_metal_encapsulaion_flange_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 0.0,
                    2. * CLHEP::cm), // Bounding envelope already constructed
                                       // to put equator at origin
      front_metal_flange_encapsulation_log, // the logical volume
      prefix + "_encapsulation_phys",       // a name for this physical volume
      envelope_log,                             // the mother volume
      false,                                // no boolean ops
      0);                                   // copy number
  rear_metal_encapsulation_flange_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 0.0,
                    -0.2 * CLHEP::cm), // Bounding envelope already constructed
                                        // to put equator at origin
      rear_metal_flange_encapsulation_log, // the logical volume
      prefix + "_encapsulation_phys",      // a name for this physical volume
      envelope_log,                            // the mother volume
      false,                               // no boolean ops
      0);                                  // copy number
  acrylic_flange_encapsulaion_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(1.0, 1.0,
                    0.8 * CLHEP::cm),   // Bounding envelope already constructed
                                        // to put equator at origin
      acrylic_flange_encapsulation_log, // the logical volume
      prefix + "_encapsulation_phys",   // a name for this physical volume
      envelope_log,                         // the mother volume
      false,                            // no boolean ops
      0);
  silica_bag_encapsulation_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 13.5 * CLHEP::cm,
                    -7.4 * CLHEP::cm), // Bounding envelope already constructed
                                        // to put equator at origin
      silica_bag_encapsulation_log,     // the logical volume
      prefix + "_encapsulation_phys",   // a name for this physical volume
      envelope_log,                         // the mother volume
      false,                            // no boolean ops
      0);

  /*silica_bag_encapsulation_phys2 = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, -13.5 * CLHEP::cm,
                    7.4 * CLHEP::cm), // Bounding envelope already constructed
                                        // to put equator at origin
      silica_bag_encapsulation_log,     // the logical volume
      prefix + "_encapsulation_phys",   // a name for this physical volume
      envelope_log,                         // the mother volume
      false,                            // no boolean ops
      0);*/

  cable_encapsulation_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(0.0, 0.0,
                    -15.2 * CLHEP::cm), // Bounding envelope already constructed
                                      // to put equator at origin
      cable_encapsulation_log,        // the logical volume
      prefix + "_encapsulation_phys", // a name for this physical volume
      envelope_log,                       // the mother volume
      false,                          // no boolean ops
      0);

  // place inner solids in outer solid (vacuum)
  inner1_phys = new G4PVPlacement(
      0,                                   // no rotation
      G4ThreeVector(0.0, 0.0, 2. * hhgap), // puts face equator in right place,
                                           // in front of tolerance gap
      inner1_log,                          // the logical volume
      prefix + "_inner1_phys",             // a name for this physical volume
      body_log,                            // the mother volume
      false,                               // no boolean ops
      0);                                  // copy number

  inner2_phys = new G4PVPlacement(
      0,             // no rotation
      noTranslation, // puts face equator in right place, behind the tolerance
                     // gap
      inner2_log,    // the logical volume
      prefix + "_inner2_phys", // a name for this physical volume
      body_log,                // the mother volume
      false,                   // no boolean ops
      0);                      // copy number

  // place gap between inner1 and inner2
  central_gap_phys = new G4PVPlacement(
      0, // no rotation
      G4ThreeVector(
          0.0, 0.0,
          hhgap), // puts face equator in right place, between inner1 and inner2
      central_gap_log,              // the logical volume
      prefix + "_central_gap_phys", // a name for this physical volume
      body_log,                     // the mother volume
      false,                        // no boolean ops
      0);                           // copy number

  // place dynode in stem/back
  dynode_phys = new G4PVPlacement(
      0, G4ThreeVector(0.0, 0.0, fParams.dynodeTop - hhDynode),
      prefix + "_dynode_phys", dynode_log, inner2_phys, false, 0);

  /// Encapsulation
  // G4RotationMatrix* pmtrot = new G4RotationMatrix();
  // pmtrot->rotateY(angle_y);
  // pmtrot->rotateX(angle_x);
  // G4ThreeVector offsetfrontencapsulation = G4ThreeVector(0.0, 0.0,
  // 0.8*CLHEP::cm); G4ThreeVector offsetfrontencapsulation_rot =
  // pmtrot->inverse()(offsetfrontencapsulation);
  // G4ThreeVector frontencapsulationpos = pmtpos + offsetacrylicflange_rot +
  // offsetfrontencapsulation_rot;

  // build the optical surface for the dynode straight away since we already
  // have the logical volume
  new G4LogicalSkinSurface(prefix + "_dynode_logsurf", dynode_log,
                           fParams.dynode_surface);

  // Add the encapsulation surfaces
  new G4LogicalSkinSurface(
      "in_encapsulation_skin",
      in_encapsulation_log,              // Logical Volume
      fParams.in_encapsulation_surface); // Surface Property
  new G4LogicalSkinSurface(
      "optical_gel_encapsulation_skin",
      optical_gel_encapsulation_log,              // Logical Volume
      fParams.optical_gel_surface); // Surface Property
  new G4LogicalSkinSurface(
      "front_encapsulation_skin",
      front_encapsulation_log,              // Logical Volume
      fParams.front_encapsulation_surface); // Surface Property
  new G4LogicalSkinSurface(
      "rear_encapsulation_skin",
      rear_encapsulation_log,              // Logical Volume
      fParams.rear_encapsulation_surface); // Surface Property
  new G4LogicalSkinSurface(
      "front_metal_flange_encapsulation_skin",
      front_metal_flange_encapsulation_log, /// Logical Volume
      fParams.metal_flange_surface);        // Surface Property
  new G4LogicalSkinSurface(
      "rear_metal_flange_encapsulation_skin",
      rear_metal_flange_encapsulation_log, /// Logical Volume
      fParams.metal_flange_surface);       // Surface Property
  new G4LogicalSkinSurface("acrylic_flange_encapsulation_skin",
                           acrylic_flange_encapsulation_log, /// Logical Volume
                           fParams.acrylic_flange_surface);  // Surface Property
  new G4LogicalSkinSurface("cable_encapsulation_skin",
                           cable_encapsulation_log, /// Logical Volume
                           fParams.cable_surface);  // Surface Property
  new G4LogicalSkinSurface("silica_bag_encapsulation_skin",
                           silica_bag_encapsulation_log, /// Logical Volume
                           fParams.silica_bag_surface);  // Surface Property

  //--------------Exterior Optical Surface-----------------
  // If we're using an envelope, body_phys has been created and we can therefore
  // set the optical surfaces, otherwise this must be done later once the
  // physical volume has been placed
  if (fParams.useEnvelope) {
    // build the mirrored surface
    new G4LogicalBorderSurface(prefix + "_mirror_logsurf1", inner2_phys,
                               body_phys, fParams.mirror);
    new G4LogicalBorderSurface(prefix + "_mirror_logsurf2", body_phys,
                               inner2_phys, fParams.mirror);

    // also include the tolerance gap
    new G4LogicalBorderSurface(prefix + "_central_gap_logsurf1",
                               central_gap_phys, body_phys, fParams.mirror);
    new G4LogicalBorderSurface(prefix + "_central_gap_logsurf2", body_phys,
                               central_gap_phys, fParams.mirror);

    // photocathode surface
    new G4LogicalBorderSurface(prefix + "_photocathode_logsurf1", inner1_phys,
                               body_phys, fParams.photocathode);

    //encapsulation surface 
    new G4LogicalBorderSurface(prefix + "_backencap_logsurf1", rear_encapsulation_phys,
                               envelope_phys, fParams.rear_encapsulation_surface);
  }

  // FIXME if fParams.seEnvelope == false this can't be done yet...
  // Go ahead and place the cathode optical surface---this can always be done at
  // this point
  // ------------ FastSimulationModel -------------
  // 28-Jul-2006 WGS: Must define a G4Region for Fast Simulations
  // (change from Geant 4.7 to Geant 4.8).
  G4Region *body_region = new G4Region(prefix + "_GLG4_PMTOpticalRegion");
  body_region->AddRootLogicalVolume(body_log);
  /*GLG4PMTOpticalModel * pmtOpticalModel =*/
  new GLG4PMTOpticalModel(
      prefix + "_optical_model", body_region, body_log, fParams.photocathode,
      fParams.efficiencyCorrection, fParams.dynodeTop, fParams.dynodeRadius,
      0.0, /*prepusling handled after absorption*/
      fParams.photocathode_MINrho, fParams.photocathode_MAXrho);

  // ------------ Vis Attributes -------------
  G4VisAttributes *visAtt;
  if (fParams.simpleVis) {
    visAtt = new G4VisAttributes(G4Color(0.0, 1.0, 1.0, 0.05));
    if (fParams.useEnvelope) {
     // envelope_log->SetVisAttributes(visAtt);
    }
    body_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    dynode_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    inner1_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    inner2_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    central_gap_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    in_encapsulation_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    optical_gel_encapsulation_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    front_encapsulation_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    rear_encapsulation_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    front_metal_flange_encapsulation_log->SetVisAttributes(
        G4VisAttributes::GetInvisible());
    rear_metal_flange_encapsulation_log->SetVisAttributes(
        G4VisAttributes::GetInvisible());
    acrylic_flange_encapsulation_log->SetVisAttributes(
        G4VisAttributes::GetInvisible());
    cable_encapsulation_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    silica_bag_encapsulation_log->SetVisAttributes(
        G4VisAttributes::GetInvisible());

  } else {
    if (fParams.useEnvelope) {
      envelope_log->SetVisAttributes(G4VisAttributes::GetInvisible());
      //visAtt = new G4VisAttributes(G4Color(0.0, 1.0, 0.0, 1.0));
      //envelope_log->SetVisAttributes(visAtt);
    }
    // PMT glass
    visAtt = new G4VisAttributes(G4Color(0.0, 1.0, 1.0, 0.05));
    body_log->SetVisAttributes(visAtt);
    // dynode is medium gray
    visAtt = new G4VisAttributes(G4Color(0.5, 0.5, 0.5, 1.0));
    dynode_log->SetVisAttributes(visAtt);
    // (surface of) interior vacuum is clear orangish gray on top (PC),
    // silvery blue on bottom (mirror)
    visAtt = new G4VisAttributes(G4Color(0.7, 0.5, 0.3, 0.27));
    inner1_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0.6, 0.7, 0.8, 0.67));
    inner2_log->SetVisAttributes(visAtt);
    // central gap is invisible
    central_gap_log->SetVisAttributes(G4VisAttributes::GetInvisible());
    // set encap
    visAtt = new G4VisAttributes(G4Color(0.77, 0.97, 1., 0.3));
    in_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0.87, 0.81, 0.11, 0.5));
    optical_gel_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0.44, 0.52, 0.78, 0.5));
    front_encapsulation_log->SetVisAttributes(visAtt);
    acrylic_flange_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0., 0.03, 0.14, 1.));
    rear_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0.68, 0.68, 0.68, 1.0));
    front_metal_flange_encapsulation_log->SetVisAttributes(visAtt);
    rear_metal_flange_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(0.3, 0.1, 0.1, 1.0));
    silica_bag_encapsulation_log->SetVisAttributes(visAtt);
    visAtt = new G4VisAttributes(G4Color(1.0, 0.18, 0.8, 1.0));
    cable_encapsulation_log->SetVisAttributes(visAtt);
  }

  log_pmt = envelope_log;//body_log;

  // if using envelope place waveguide now
  if (fParams.useEnvelope && fWaveguideFactory) {
    fWaveguideFactory->SetPMTBodySolid(body_solid);
    G4LogicalVolume *log_wg = fWaveguideFactory->Construct(
        prefix + "_waveguide_log", log_pmt, fParams.simpleVis);
    G4ThreeVector offsetWg = fWaveguideFactory->GetPlacementOffset();
    new G4PVPlacement(0, // no rotation
                      offsetWg,
                      log_wg, // the logical volume
                      prefix +
                          "_waveguide_phys", // a name for this physical volume
                      envelope_log,          // the mother volume
                      false,                 // no boolean ops
                      0);                    // copy number
  }

  return log_pmt;
}

G4VSolid *EncapsulatedPMTConstruction::BuildSolid(const std::string &_name) {
  GLG4TorusStack *body = new GLG4TorusStack(_name);
  body->SetAllParameters(fParams.zOrigin.size(), &fParams.zEdge[0],
                         &fParams.rhoEdge[0], &fParams.zOrigin[0]);
  return body;
}

G4PVPlacement *EncapsulatedPMTConstruction::PlacePMT(
    G4RotationMatrix *pmtrot, G4ThreeVector pmtpos, const std::string &_name,
    G4LogicalVolume *logi_pmt, G4VPhysicalVolume *mother_phys,
    bool booleanSolid, int copyNo) {
  if (fParams.useEnvelope) {
    return new G4PVPlacement(pmtrot, pmtpos, _name, logi_pmt, mother_phys,
                             booleanSolid, copyNo);
  } else {
    encapsulation_phys = new G4PVPlacement(pmtrot, pmtpos, _name, logi_pmt,
                                           mother_phys, booleanSolid, copyNo);

    body_phys = new G4PVPlacement(pmtrot, pmtpos, _name, logi_pmt, mother_phys,
                                  booleanSolid, copyNo);

    // build the mirrored surface
    new G4LogicalBorderSurface(_name + "_mirror_logsurf1", inner2_phys,
                               body_phys, fParams.mirror);
    new G4LogicalBorderSurface(_name + "_mirror_logsurf2", body_phys,
                               inner2_phys, fParams.mirror);

    // also include the tolerance gap
    new G4LogicalBorderSurface(_name + "_central_gap_logsurf1",
                               central_gap_phys, body_phys, fParams.mirror);
    new G4LogicalBorderSurface(_name + "_central_gap_logsurf2", body_phys,
                               central_gap_phys, fParams.mirror);

    // photocathode surface
    new G4LogicalBorderSurface(_name + "_photocathode_logsurf1", inner1_phys,
                               body_phys, fParams.photocathode);

    // if not using envelope place waveguide now
    if (fWaveguideFactory) {
      G4LogicalVolume *log_wg = fWaveguideFactory->Construct(
          _name + "_waveguide_log", logi_pmt, fParams.simpleVis);
      // pmtrot is a passive rotation, but we need an active one to put offsetWg
      // into coordinates of mother
      G4ThreeVector offsetWg = fWaveguideFactory->GetPlacementOffset();
      G4ThreeVector offsetWg_rot = pmtrot->inverse()(offsetWg);
      G4ThreeVector waveguidepos = pmtpos + offsetWg_rot;
      new G4PVPlacement(pmtrot, waveguidepos,
                        _name + "_waveguide", // a name for this physical volume
                        log_wg,               // the logical volume
                        mother_phys,          // the mother volume
                        false,                // no boolean ops
                        0);                   // copy number
    }
    return body_phys;
  }
}

G4VSolid *
EncapsulatedPMTConstruction::optical_gel_height_subtraction(const std::string &_name) {

  double enc_radius = 20.0;   // default radius
  double enc_thickness = 0.8;
  G4Sphere *optical_gel_1 = 
                    new G4Sphere("optical_gel_1_encapsulation_solid",
                    15*CLHEP::cm,///(enc_radius-enc_thickness)*CLHEP::cm,                   // rmin 20 cm
                    20.0*CLHEP::cm,// (enc_radius+enc_thickness)+5 * CLHEP::cm, // rmax: 20.8 cm
                    0.5 * CLHEP::pi, CLHEP::twopi,            // phi
                    0., 0.5 * CLHEP::pi); 
  G4Tubs *gel_subtract =
      new G4Tubs("gel_sub__solid", 0.0,
                  25*CLHEP::cm, // solid cylinder (FIXME?)
                  15*CLHEP::cm,             // half height of cylinder
                 0., CLHEP::twopi);    // cylinder complete in phi
  
  return new G4SubtractionSolid(_name, optical_gel_1, gel_subtract, 0, G4ThreeVector(0.0, 0.0, 0.0*CLHEP::cm)); //8.5
  //return inner_encapsulation_solid2;
}//


G4VSolid *
EncapsulatedPMTConstruction::optical_gel_pmt_subtraction(const std::string &_name, GLG4TorusStack *body){

   G4VSolid *optical_gel_2 = 0; 
   optical_gel_2 = optical_gel_height_subtraction("temp_gel"+ _name); 


  return new G4SubtractionSolid(_name, optical_gel_2, body, 0, G4ThreeVector(0.0, 0.0, 9.8*CLHEP::cm)); //8.5
  //return inner_encapsulation_solid2;
}//

G4VSolid *
EncapsulatedPMTConstruction::NewEnvelopeSolid(const std::string &_name) {
  /*G4double zTop = fParams.zEdge[0] + fParams.faceGap;
  G4double zBottom = fParams.zEdge[fParams.zEdge.size() - 1];
  G4double rho = fParams.minEnvelopeRadius;
  for (unsigned i = 0; i < fParams.rhoEdge.size(); i++) {
    if (fParams.rhoEdge[i] > rho) {
      rho = fParams.rhoEdge[i];
    }
  }

  G4double mainCylHalfHeight = std::max(zTop, -zBottom);
  G4double subCylHalfHeight =
      (mainCylHalfHeight - std::min(zTop, -zBottom)) / 2.0;
  G4double subCylOffset;
  if (zTop < -zBottom) {
    subCylOffset = zTop + subCylHalfHeight;
  } else {
    subCylOffset = zBottom - subCylHalfHeight;
  }

  G4Tubs *mainEnvelope = new G4Tubs(_name + "_main", 0.0, rho,
                                    mainCylHalfHeight, 0.0, CLHEP::twopi);
  G4Tubs *subEnvelope = new G4Tubs(_name + "_sub", 0.0, rho * 1.1,
                                   subCylHalfHeight, 0.0, CLHEP::twopi);

  return new G4SubtractionSolid(_name, mainEnvelope, subEnvelope, 0,
                                G4ThreeVector(0.0, 0.0, subCylOffset));*/

  G4Sphere *outer_s = new G4Sphere(_name+ "_main", 0. * CLHEP::mm, 25.4*CLHEP::cm, 0.0, CLHEP::twopi, 0.0, CLHEP::twopi);
  //G4Sphere *inner_s = new G4Sphere(_name+ "_sub", 0. * CLHEP::mm, 25.3*CLHEP::cm, 0.0, CLHEP::twopi, 0.0, CLHEP::twopi);

  //return new G4SubtractionSolid(_name, outer_s, inner_s);
  return outer_s;
}

void EncapsulatedPMTConstruction::CalcInnerParams(
    GLG4TorusStack *body, std::vector<double> &innerZEdge,
    std::vector<double> &innerRhoEdge, int &equatorIndex,
    double &zLowestDynode) {

  ////// The encapsulation size is hard coded in to insure the correct size for
  /// BUTTON encapsulation but it could be added here if different pmt sizes
  /// were wanted!

  // Local references
  const G4double wall = fParams.wallThickness;
  const G4double dynodeRadius = fParams.dynodeRadius;
  const std::vector<G4double> &outerZEdge = fParams.zEdge;
  const std::vector<G4double> &outerRhoEdge = fParams.rhoEdge;
  const int nEdge = fParams.zEdge.size() - 1;

  // set shapes of inner volumes, scan for lowest allowed point of dynode
  zLowestDynode = fParams.dynodeTop;
  innerZEdge.resize(fParams.zEdge.size());
  innerRhoEdge.resize(fParams.rhoEdge.size());

  // We will have to calculate the inner dimensions of the PMT.
  G4ThreeVector norm;
  equatorIndex = -1;

  // calculate inner surface edges, check dynode position, and find equator
  innerZEdge[0] = outerZEdge[0] - wall;
  innerRhoEdge[0] = 0.0;
  for (int i = 1; i < nEdge; i++) {
    norm =
        body->SurfaceNormal(G4ThreeVector(0.0, outerRhoEdge[i], outerZEdge[i]));
    innerZEdge[i] = outerZEdge[i] - wall * norm.z();
    innerRhoEdge[i] = outerRhoEdge[i] - wall * norm.y();
    if (innerRhoEdge[i] > dynodeRadius && innerZEdge[i] < zLowestDynode) {
      zLowestDynode = innerZEdge[i];
    }
    if (outerZEdge[i] == 0.0 || innerZEdge[i] == 0.0) {
      equatorIndex = i;
    }
  }

  innerZEdge[nEdge] = outerZEdge[nEdge] + wall;
  innerRhoEdge[nEdge] = outerRhoEdge[nEdge] - wall;

  // one final check on dynode allowed position
  if (innerRhoEdge[nEdge] > dynodeRadius && innerZEdge[nEdge] < zLowestDynode) {
    zLowestDynode = innerZEdge[nEdge];
  }

  // sanity check equator index
  if (equatorIndex < 0) {
    Log::Die("EncapsulatedPMTConstruction::CalcInnerParams: Pathological PMT "
             "shape with no equator edge");
  }
  // sanity check on dynode height
  if (fParams.dynodeTop > innerZEdge[equatorIndex]) {
    Log::Die("EncapsulatedPMTConstruction::CalcInnerParams: Top of PMT dynode "
             "cannot be higher than equator.");
  }
}

} // namespace RAT
