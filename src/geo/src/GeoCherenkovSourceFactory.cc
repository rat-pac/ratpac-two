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
#include <RAT/GeoCherenkovSourceFactory.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <string>
#include <vector>

namespace RAT {

G4VPhysicalVolume *GeoCherenkovSourceFactory::Construct(DBLinkPtr table) {
  info << "Building Cherenkov Source" << newline;
  const std::string volumeName = table->GetIndex();
  const std::string motherName = table->GetS("mother");

  // Acrylic properties
  G4Material *sourceMaterial = G4Material::GetMaterial(table->GetS("source_material"));
  const std::vector<double> &sourceColor = table->GetDArray("source_vis_color");

  // Acrylic neck
  const double neckOd = table->GetD("neck_od");
  const double neckOdLen = table->GetD("neck_od_len");
  const double neckId = table->GetD("neck_id");
  const double neckIdLen = table->GetD("neck_id_len");

  // Acrylic ball
  const double ballOd = table->GetD("ball_od");
  const double ballId = table->GetD("ball_id");

  // Decay chamber
  G4Material *decayChamberMaterial =
      G4Material::GetMaterial(table->GetS("decay_chamber_material"));  // inside the decay chamber
  const std::vector<double> &decayChamberColor = table->GetDArray("decay_chamber_vis_color");

  // Black paint layer
  G4Material *blackPaintMaterial = G4Material::GetMaterial(table->GetS("black_paint_material"));
  const double blackPaintThickness = table->GetD("black_paint_thickness");
  const std::vector<double> &blackPaintColor = table->GetDArray("black_paint_vis_color");

  // White paint layer
  G4Material *whitePaintMaterial = G4Material::GetMaterial(table->GetS("white_paint_material"));
  const double whitePaintThickness = table->GetD("white_paint_thickness");
  const std::vector<double> &whitePaintColor = table->GetDArray("white_paint_vis_color");

  // Surface properties
  G4OpticalSurface *paintSurface = GetSurface(table->GetS("paint_surface"));

  // Neck sleeve
  G4Material *sleeveMaterial = G4Material::GetMaterial(table->GetS("sleeve_material"));
  const std::vector<double> &sleeveColor = table->GetDArray("sleeve_vis_color");

  // PMT window
  G4Material *windowMaterial = G4Material::GetMaterial(table->GetS("window_material"));
  const double windowThickness = table->GetD("window_thickness");
  const double windowOd1 = table->GetD("window_od1");
  const double windowOd1Len = table->GetD("window_od1_len");
  const double windowOd2 = table->GetD("window_od2");
  const double windowOd2Len = table->GetD("window_od2_len");
  const std::vector<double> &windowColor = table->GetDArray("window_vis_color");

  // Gas inlet/outlet tubes
  G4Material *tubeMaterial = G4Material::GetMaterial(table->GetS("tube_material"));
  const double tubeId = table->GetD("tube_id");
  const double tubeOd = table->GetD("tube_od");
  const double tubeLength = table->GetD("tube_length");
  const double tubeOffset = table->GetD("tube_offset");
  const double tubeRot = table->GetD("tube_rot");
  const std::vector<double> &tubeColor = table->GetDArray("tube_vis_color");

  // Can stuff
  G4Material *canMaterial = G4Material::GetMaterial(table->GetS("can_material"));
  const double canOd1 = table->GetD("can_od1");
  const double canOd1Len = table->GetD("can_od1_len");
  const double canOd2 = table->GetD("can_od2");
  const double canOd2Len = table->GetD("can_od2_len");
  const double canOd3 = table->GetD("can_od3");
  const double canOd3Len = table->GetD("can_od3_len");
  const double canOd4 = table->GetD("can_od4");
  const double canOd4Len = table->GetD("can_od4_len");
  const double canId1 = table->GetD("can_id1");
  const double canId1Len = table->GetD("can_id1_len");
  const double canId2 = table->GetD("can_id2");
  const double canId2Len = table->GetD("can_id2_len");
  const double canId3 = table->GetD("can_id3");
  const double canId3Len = table->GetD("can_id3_len");
  const double canId4 = table->GetD("can_id4");
  const double canId4Len = table->GetD("can_id4_len");
  const std::vector<double> &canColor = table->GetDArray("can_vis_color");

  // PMT stuff
  const std::string pmtType = table->GetS("pmt_model");
  const std::vector<double> &pmtColor = table->GetDArray("pmt_vis_color");
  const double pmtRadius = table->GetD("pmt_radius");
  const double pmtHeight = table->GetD("pmt_height");

  // Calculate the z location of the start of the neck
  double neckStartZ = sqrt(ballId * ballId / 4.0 - neckId * neckId / 4.0);

  // position of the neck relative to the decay chamber origin
  G4ThreeVector neckPos(0.0, 0.0, neckStartZ + neckIdLen / 2.0);

  // Constructs the sleeveSolid - aluminium sleeve in the neck - to be placed in
  // world
  G4Tubs *sleeveSolid = new G4Tubs(volumeName + "SleeveSolid", 0, neckId / 2.0, neckIdLen / 2.0, 0, CLHEP::twopi);

  // Construct windowSolid - to be placed in sleeveSolid
  G4Tubs *window1 =
      new G4Tubs(volumeName + "window1", 0, windowOd1 / 2.0, (windowOd1Len + windowOd2Len) / 2, 0, CLHEP::twopi);
  G4Tubs *window2 = new G4Tubs(volumeName + "window2", 0, windowOd2 / 2.0, (windowOd2Len) / 2, 0, CLHEP::twopi);
  G4ThreeVector win2Offset(0.0, 0.0, (windowOd1Len + windowOd2Len) / 2.0 - windowOd2Len / 2.0);
  G4UnionSolid *windowFilled = new G4UnionSolid(volumeName + "WindowFilled", window1, window2, NULL, win2Offset);
  double knockoutThickness = windowOd1Len + windowOd2Len - windowThickness;
  G4Tubs *windowKnockout = new G4Tubs(volumeName + "WindowKnockout", 0, pmtRadius, (knockoutThickness) / 2.0, 0,
                                      CLHEP::twopi);  // leave room for the pmt
  G4ThreeVector windowKnockoutPos(0.0, 0.0, (windowOd1Len + windowOd2Len) / 2.0 - knockoutThickness / 2.0);
  G4SubtractionSolid *windowSolid =
      new G4SubtractionSolid(volumeName + "WindowSolid", windowFilled, windowKnockout, NULL, windowKnockoutPos);

  // Position of window relative to neck sleeve origin
  G4ThreeVector windowPos(0.0, 0.0, (windowOd1Len + windowOd2Len) / 2.0 - neckIdLen / 2.0);

  // Construct the gas inlet/outlet tubes - to be placed in world
  G4Tubs *tubeSolid = new G4Tubs(volumeName + "Tube", 0.0, tubeOd / 2.0, tubeLength / 2.0, 0, CLHEP::twopi);
  G4Tubs *pipeSolid = new G4Tubs(volumeName + "Tube", tubeId / 2.0, tubeOd / 2.0, tubeLength / 2.0, 0,
                                 CLHEP::twopi);  // to be placed inside tube
  G4RotationMatrix *inletRot = new G4RotationMatrix();
  inletRot->rotateY(0.0);
  inletRot->rotateX(tubeRot / 180.0 * CLHEP::pi);
  inletRot->rotateZ(0.0);
  G4ThreeVector inletPos(0.0, tubeOffset + neckId / 2.0, neckStartZ + neckIdLen / 2.0);
  G4RotationMatrix *outletRot = new G4RotationMatrix();
  outletRot->rotateY(0.0);
  outletRot->rotateX(-tubeRot / 180.0 * CLHEP::pi);
  outletRot->rotateZ(0.0);
  G4ThreeVector outletPos(0.0, -tubeOffset - neckId / 2.0, neckStartZ + neckIdLen / 2.0);

  // Construct the sourceSolid - the bulk of acrylic - to be placed in world
  G4Tubs *sourceOuterNeck =
      new G4Tubs(volumeName + "SourceOuterNeck", canOd1 / 2.0, neckOd / 2.0, neckOdLen / 2.0, 0, CLHEP::twopi);
  G4Tubs *sourceInnerNeck =
      new G4Tubs(volumeName + "SourceInnerNeck", 0.0, neckOd / 2.0, neckIdLen / 2.0, 0, CLHEP::twopi);
  G4ThreeVector outerOffset(0.0, 0.0, neckOdLen - neckIdLen);
  G4UnionSolid *sourceNeck =
      new G4UnionSolid(volumeName + "SourceNeck", sourceInnerNeck, sourceOuterNeck, NULL, outerOffset);
  G4Orb *sourceBall = new G4Orb(volumeName + "SourceBall", ballOd / 2.0);
  G4UnionSolid *sourceFilledNeck = new G4UnionSolid(volumeName + "SourceFilled", sourceBall, sourceNeck, NULL, neckPos);
  G4SubtractionSolid *sourceFilledTubes = new G4SubtractionSolid(volumeName + "SourceFilledTubes", sourceFilledNeck,
                                                                 sleeveSolid, NULL, neckPos);  // remove PMT volume
  G4SubtractionSolid *sourceFilledOutlet =
      new G4SubtractionSolid(volumeName + "SourceFilledOutlet", sourceFilledTubes, tubeSolid, inletRot,
                             inletPos);  // remove inlet volume
  G4SubtractionSolid *sourceSolid =
      new G4SubtractionSolid(volumeName + "SourceSolid", sourceFilledOutlet, tubeSolid, outletRot,
                             outletPos);  // remove outlet volume

  // Construct the blackPaintSolid - to be placed in sourceSolid
  G4Orb *paintOverlap = new G4Orb(volumeName + "BlackPaintOverlap", ballId / 2.0);
  G4SubtractionSolid *blackPaintTubes = new G4SubtractionSolid(volumeName + "blackPaintTubes", paintOverlap,
                                                               sleeveSolid, NULL, neckPos);  // remove PMT volume
  G4SubtractionSolid *blackPaintOutlet =
      new G4SubtractionSolid(volumeName + "blackPaintOutlet", blackPaintTubes, tubeSolid, inletRot,
                             inletPos);  // remove inlet volume
  G4SubtractionSolid *blackPaintSolid =
      new G4SubtractionSolid(volumeName + "blackPaintSolid", blackPaintOutlet, tubeSolid, outletRot,
                             outletPos);  // remove outlet volume

  // Construct the whitePaintSolid - to be placed in blackPaintSolid
  G4Orb *tpbOverlap = new G4Orb(volumeName + "WhitePaintOverlap", ballId / 2.0 - blackPaintThickness);
  G4SubtractionSolid *whitePaintTubes = new G4SubtractionSolid(volumeName + "whitePaintTubes", tpbOverlap, sleeveSolid,
                                                               NULL, neckPos);  // remove PMT volume
  G4SubtractionSolid *whitePaintOutlet =
      new G4SubtractionSolid(volumeName + "whitePaintOutlet", whitePaintTubes, tubeSolid, inletRot,
                             inletPos);  // remove inlet volume
  G4SubtractionSolid *whitePaintSolid =
      new G4SubtractionSolid(volumeName + "whitePaintSolid", whitePaintOutlet, tubeSolid, outletRot,
                             outletPos);  // remove outlet volume

  // Constructs the decayChamberSolid - this is the He+Li8 - to be placed in
  // whitePaintSolid
  G4Orb *decayChamberOverlap =
      new G4Orb(volumeName + "DecayChamberOverlap", ballId / 2.0 - blackPaintThickness - whitePaintThickness);
  G4SubtractionSolid *decayChamberTubes =
      new G4SubtractionSolid(volumeName + "DecayChamberTubes", decayChamberOverlap, sleeveSolid, NULL,
                             neckPos);  // remove PMT volume
  G4SubtractionSolid *decayChamberOutlet =
      new G4SubtractionSolid(volumeName + "DecayChamberOutlet", decayChamberTubes, tubeSolid, inletRot,
                             inletPos);  // remove inlet volume
  G4SubtractionSolid *decayChamberSolid = new G4SubtractionSolid(
      volumeName + "DecayChamberSolid", decayChamberOutlet, tubeSolid, outletRot, outletPos);  // remove outlet volume

  // Construct the can solid... carefully (+1.0 fudge factors included where
  // necessary) - to be placed in world
  double canHalflen = (canOd1Len + canOd2Len + canOd3Len + canOd4Len) / 2.0;
  // OD1 is the biggest so build it first and remove other pieces
  G4Tubs *canTube = new G4Tubs(volumeName + "CanTube", 0, canOd1 / 2.0, canHalflen, 0, CLHEP::twopi);
  // remove outer 2
  G4Tubs *canRmO2 =
      new G4Tubs(volumeName + "CanRmO2", canOd2 / 2.0, canOd1 / 2.0 + 1.0, canOd2Len / 2.0, 0, CLHEP::twopi);
  G4ThreeVector rmO2Pos(0, 0, canOd1Len + canOd2Len / 2.0 - canHalflen);
  G4SubtractionSolid *canSans2 = new G4SubtractionSolid(volumeName + "CanSans2", canTube, canRmO2, NULL, rmO2Pos);
  // remove outer 3
  G4Tubs *canRmO3 =
      new G4Tubs(volumeName + "CanRmO3", canOd3 / 2.0, canOd1 / 2.0 + 1.0, canOd3Len / 2.0 + 1.0, 0, CLHEP::twopi);
  G4ThreeVector rmO3Pos(0, 0, canOd1Len + canOd2Len + canOd3Len / 2.0 - canHalflen);
  G4SubtractionSolid *canSans2Sans3 =
      new G4SubtractionSolid(volumeName + "CanSans2Sans3", canSans2, canRmO3, NULL, rmO3Pos);
  // remove outer 4
  G4Tubs *canRmO4 =
      new G4Tubs(volumeName + "CanRmO4", canOd4 / 2.0, canOd1 / 2.0 + 1.0, canOd4Len / 2.0 + 1.0, 0, CLHEP::twopi);
  G4ThreeVector rmO4Pos(0, 0, canHalflen - canOd4Len / 2.0 + 1.0);
  G4SubtractionSolid *canFilled =
      new G4SubtractionSolid(volumeName + "CanFilled", canSans2Sans3, canRmO4, NULL, rmO4Pos);
  // remove inner 1
  G4Tubs *canRmI1 = new G4Tubs(volumeName + "CanRmI1", 0, canId1 / 2.0, canId1Len / 2.0 + 1.0, 0, CLHEP::twopi);
  G4ThreeVector rmI1Pos(0, 0, canId1Len / 2.0 - canHalflen);
  G4SubtractionSolid *canFilledSans1 =
      new G4SubtractionSolid(volumeName + "CanFilledSans1", canFilled, canRmI1, NULL, rmI1Pos);
  // remove inner 2
  G4Tubs *canRmI2 = new G4Tubs(volumeName + "CanRmI2", 0, canId2 / 2.0, canId2Len / 2.0 + 1.0, 0, CLHEP::twopi);
  G4ThreeVector rmI2Pos(0, 0, canId1Len + canId2Len / 2.0 - canHalflen + 1.0);
  G4SubtractionSolid *canFilledSans1Sans2 =
      new G4SubtractionSolid(volumeName + "CanFilledSans1Sans2", canFilledSans1, canRmI2, NULL, rmI2Pos);
  // remove inner 3
  G4Tubs *canRmI3 = new G4Tubs(volumeName + "CanRmI3", 0, canId3 / 2.0, canId3Len / 2.0, 0, CLHEP::twopi);
  G4ThreeVector rmI3Pos(0, 0, canHalflen - canId4Len - canId3Len / 2.0);
  G4SubtractionSolid *canFilledSans1Sans2Sans3 =
      new G4SubtractionSolid(volumeName + "CanFilledSans1Sans2Sans3", canFilledSans1Sans2, canRmI3, NULL, rmI3Pos);
  // remove inner 4
  G4Tubs *canRmI4 = new G4Tubs(volumeName + "CanRmI4", 0, canId4 / 2.0, canId4Len / 2.0 + 1, 0, CLHEP::twopi);
  G4ThreeVector rmI4Pos(0, 0, canHalflen - canId4Len / 2.0);
  G4SubtractionSolid *canSolid =
      new G4SubtractionSolid(volumeName + "CanFilledSans1Sans2Sans3", canFilledSans1Sans2Sans3, canRmI4, NULL, rmI4Pos);

  // Position of the can relative to the decay chamber origin
  G4ThreeVector canPos(0.0, 0.0, neckStartZ + neckIdLen + canHalflen);

  // Bind constructed solids to their materials
  G4LogicalVolume *canLog = new G4LogicalVolume(canSolid, canMaterial, volumeName + "_Can");
  G4LogicalVolume *sourceLog = new G4LogicalVolume(sourceSolid, sourceMaterial, volumeName + "_Acrylic");
  G4LogicalVolume *inletTubeLog = new G4LogicalVolume(tubeSolid, decayChamberMaterial, volumeName + "_InletTube");
  G4LogicalVolume *inletPipeLog = new G4LogicalVolume(pipeSolid, tubeMaterial, volumeName + "_InletPipe");
  G4LogicalVolume *outletTubeLog = new G4LogicalVolume(tubeSolid, decayChamberMaterial, volumeName + "_OutletTube");
  G4LogicalVolume *outletPipeLog = new G4LogicalVolume(pipeSolid, tubeMaterial, volumeName + "_OutletPipe");
  G4LogicalVolume *sleeveLog = new G4LogicalVolume(sleeveSolid, sleeveMaterial, volumeName + "_Sleeve");
  G4LogicalVolume *windowLog = new G4LogicalVolume(windowSolid, windowMaterial, volumeName + "_Window");
  G4LogicalVolume *blackPaintLog = new G4LogicalVolume(blackPaintSolid, blackPaintMaterial, volumeName + "_BlackPaint");
  G4LogicalVolume *whitePaintLog = new G4LogicalVolume(whitePaintSolid, whitePaintMaterial, volumeName + "_WhitePaint");
  G4LogicalVolume *decayChamberLog =
      new G4LogicalVolume(decayChamberSolid, decayChamberMaterial, volumeName + "_DecayChamber");

  // Assemble the PMT
  //    Initialise(volumeName, windowMaterial); //FIXME - what is this material
  //    for
  G4VisAttributes *attrib = GetColor(pmtColor);
  attrib->SetForceSolid(true);
  //    PMTConstructorParams params(pmtType, attrib);
  //    ConstructPMT(0, params);
  //    AssembleFullPMT(0, attrib);
  //    SetupFullOpticalModel(0);
  //    std::string pmtName = volumeName + "_pmtenv_" + ::to_string(pmtLcn);
  //    //Necessary naming for PMTOpticalModel
  //
  //    //Calculate where to put and how to place the PMT
  G4ThreeVector pmtPos(0, 0, pmtHeight / 2.0 - neckIdLen / 2.0 + windowThickness);
  G4ThreeVector pmtDir(0.0, 0.0,
                       1.0);  // N.B. convention is pointing the end opposite the sensitive end
  //    CorrectPMTPosAndDir(DU::PMTInfo::CALIB, pmtPos, pmtDir);
  //    //Stole this math from N16 source factory
  //    double yAngle = (-1.0)*atan2(pmtDir.x(), pmtDir.z());
  //    double xAngle = atan2(pmtDir.y(), sqrt(pmtDir.x() * pmtDir.x() +
  //    pmtDir.z() * pmtDir.z())); G4RotationMatrix* pmtRot = new
  //    G4RotationMatrix(); pmtRot->rotateY(yAngle); pmtRot->rotateX(xAngle);
  //    pmtRot->rotateZ(0.0);
  //
  //    //Set visualization properties
  attrib = GetColor(canColor);
  attrib->SetForceSolid(true);
  canLog->SetVisAttributes(attrib);
  attrib = GetColor(sourceColor);
  attrib->SetForceSolid(true);
  sourceLog->SetVisAttributes(attrib);
  attrib = GetColor(sleeveColor);
  attrib->SetForceSolid(true);
  sleeveLog->SetVisAttributes(attrib);
  attrib = GetColor(windowColor);
  attrib->SetForceSolid(true);
  windowLog->SetVisAttributes(attrib);
  attrib = GetColor(tubeColor);
  attrib->SetForceSolid(true);
  inletPipeLog->SetVisAttributes(attrib);
  outletPipeLog->SetVisAttributes(attrib);
  attrib = GetColor(blackPaintColor);
  attrib->SetForceSolid(true);
  blackPaintLog->SetVisAttributes(attrib);
  attrib = GetColor(whitePaintColor);
  attrib->SetForceSolid(true);
  whitePaintLog->SetVisAttributes(attrib);
  attrib = GetColor(decayChamberColor);
  attrib->SetForceSolid(true);
  inletTubeLog->SetVisAttributes(attrib);
  outletTubeLog->SetVisAttributes(attrib);
  decayChamberLog->SetVisAttributes(attrib);

  G4ThreeVector zero(0.0, 0.0, 0.0);

  // Put stuff in the mother volume
  G4VPhysicalVolume *motherPhys = FindPhysMother(table->GetS("mother"));

  G4RotationMatrix *rotation = motherPhys->GetObjectRotation();
  G4ThreeVector position = motherPhys->GetObjectTranslation();
  // second is rotation, first is position

  new G4PVPlacement(rotation, canPos + position, volumeName + "_Can", canLog, motherPhys, false,
                    0);  // can placed in mother
  //
  G4VPhysicalVolume *sleevePhys = new G4PVPlacement(rotation, neckPos + position, volumeName + "_Sleeve", sleeveLog,
                                                    motherPhys, false, 0);  // housing placed in mother
  /*G4VPhysicalVolume *windowPhys =*/new G4PVPlacement(NULL, windowPos, volumeName + "_Window", windowLog, sleevePhys,
                                                       false,
                                                       0);  // window placed in light shield
  // Here is the code to place the PMT in the volume with the right copy number.
  // fEnvelope[0]->GetLogicalVolume() is a bit odd, but gets the envelope
  // logical volume for the previously constructed PMT from the
  // GeoPMTBuilderBase super class.
  //    /*G4VPhysicalVolume *pmtPhys =*/ new G4PVPlacement(pmtRot, pmtPos,
  //    pmtName, fEnvelope[0]->GetLogicalVolume(), sleevePhys, false, pmtLcn);
  //    //pmt placed in housing
  //
  G4VPhysicalVolume *inletTubePhys = new G4PVPlacement(inletRot, inletPos, volumeName + "_InletTube", inletTubeLog,
                                                       motherPhys, false, 0);  // placed in mother
  /*G4VPhysicalVolume *inletPipePhys =*/new G4PVPlacement(NULL, zero, volumeName + "_InletPipe", inletPipeLog,
                                                          inletTubePhys, false,
                                                          0);  // placed in inletTube
  G4VPhysicalVolume *outletTubePhys = new G4PVPlacement(outletRot, outletPos, volumeName + "_OutletTube", outletTubeLog,
                                                        motherPhys, false, 0);  // placed in mother
  /*G4VPhysicalVolume *outletPipePhys =*/new G4PVPlacement(NULL, zero, volumeName + "_OutletPipe", outletPipeLog,
                                                           outletTubePhys, false, 0);  // placed in outletTube

  G4VPhysicalVolume *sourcePhys = new G4PVPlacement(rotation, position, volumeName + "_Acrylic", sourceLog, motherPhys,
                                                    false, 0);  // placed in mother
  G4VPhysicalVolume *blackPaintPhys = new G4PVPlacement(NULL, zero, volumeName + "_BlackPaint", blackPaintLog,
                                                        sourcePhys, false, 0);  // placed inside source
  G4VPhysicalVolume *whitePaintPhys = new G4PVPlacement(NULL, zero, volumeName + "_WhitePaint", whitePaintLog,
                                                        blackPaintPhys, false, 0);  // placed inside black paint
  G4VPhysicalVolume *decayChamberPhys = new G4PVPlacement(NULL, zero, volumeName + "_DecayChamber", decayChamberLog,
                                                          whitePaintPhys, false, 0);  // placed inside white paint

  // Apply surface roughness to layers both directions
  new G4LogicalBorderSurface(volumeName + "DecayChamberToTpb", decayChamberPhys, whitePaintPhys, paintSurface);
  new G4LogicalBorderSurface(volumeName + "TpbToDecayChamber", whitePaintPhys, decayChamberPhys, paintSurface);

  return NULL;  // This function should probably be void, fixme
}

G4OpticalSurface *GeoCherenkovSourceFactory::GetSurface(std::string surface_name) {
  if (Materials::optical_surface.count(surface_name) == 0)
    Log::Die("error: surface " + surface_name + " does not exist");
  return Materials::optical_surface[surface_name];
}

G4VisAttributes *GeoCherenkovSourceFactory::GetColor(std::vector<double> color) {
  if (color.size() == 4) {
    return new G4VisAttributes(G4Color(color[0], color[1], color[2], color[3]));
  } else if (color.size() == 3) {
    return new G4VisAttributes(G4Color(color[0], color[1], color[2]));
  } else {
    return new G4VisAttributes(G4Color());
  }
}

}  // namespace RAT
