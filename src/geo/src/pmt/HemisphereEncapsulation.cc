#include <CLHEP/Units/PhysicalConstants.h>

#include <G4Box.hh>
#include <G4GenericPolycone.hh>
#include <G4LogicalBorderSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4PVPlacement.hh>
#include <G4Paraboloid.hh>
#include <G4Region.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <G4VisAttributes.hh>
#include <RAT/HemisphereEncapsulation.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <RAT/PMTEncapsulation.hh>
#include <algorithm>

namespace RAT {

HemisphereEncapsulation::HemisphereEncapsulation(DBLinkPtr encaptable, DBLinkPtr pmttable, G4LogicalVolume *mother)
    : PMTEncapsulation("hemisphere") {
  inner_encapsulation_phys = nullptr;
  front_encapsulation_phys = nullptr;
  rear_encapsulation_phys = nullptr;

  fParams.envelope_radius = 0;  // default to 0
  try {
    fParams.envelope_radius = encaptable->GetD("envelope_radius");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.envelope_radius == 0) {
    Log::Die("Hemisphere encapsulation turned on but envelope radius not given!");
  }

  fParams.encap_radius = 0;  // default to 0
  try {
    fParams.encap_radius = encaptable->GetD("encap_radius");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.encap_radius == 0) {
    Log::Die("Hemisphere encapsulation turned on but dome radius not given!");
  }

  fParams.encap_thickness = 0;  // default to 0
  try {
    fParams.encap_thickness = encaptable->GetD("encap_thickness");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.encap_thickness == 0) {
    Log::Die("Hemisphere encapsulation turned on but dome thickness not given!");
  }

  fParams.flange_rmax = 0;  // default to 0
  try {
    fParams.flange_rmax = encaptable->GetD("flange_rmax");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.flange_rmax == 0) {
    Log::Die("Hemisphere encapsulation turned on but flange max radius not given!");
  }

  fParams.useMetalFlange = 0;  // default to no silica bag
  try {
    fParams.useMetalFlange = encaptable->GetI("use_metal_flange");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.useMetalFlange == 1) {
    front_metal_encapsulaion_flange_phys = nullptr;
    rear_metal_encapsulation_flange_phys = nullptr;
    fParams.metal_flange_material = G4Material::GetMaterial(encaptable->GetS("metal_flange_material"));
    fParams.metal_flange_surface = Materials::optical_surface[encaptable->GetS("metal_flange_material")];
    fParams.metal_flange_dimensions = encaptable->GetDArray("metal_flange_dimensions");
  }

  // PMT is only needed if optical gel is used
  fParams.useGel = 0;  // default to no gel
  std::string pmttype = pmttable->GetS("construction");

  // Check if optical gel can be used, needs toroidal
  if (pmttype == "toroidal") {
    info << "Optical gel can be used since PMT construction is toroidal" << newline;
    try {
      fParams.useGel = encaptable->GetI("use_optical_gel");
    } catch (DBNotFoundError &e) {
    };
  } else if (pmttype != "toroidal") {
    info << "Optical gel currently requires a toroidal PMT model, setting useGel = 0" << newline;
    fParams.useGel = 0;
  }

  if (fParams.useGel == 1) {
    optical_gel_encapsulation_phys = nullptr;

    // Setup PMT parameters
    fParams.zEdge = pmttable->GetDArray("z_edge");
    fParams.rhoEdge = pmttable->GetDArray("rho_edge");
    fParams.zOrigin = pmttable->GetDArray("z_origin");
    // Build PMT shape
    assert(fParams.zEdge.size() == fParams.rhoEdge.size());
    assert(fParams.zEdge.size() == fParams.zOrigin.size() + 1);

    assert(fParams.exterior_material);
    fParams.optical_gel_material = G4Material::GetMaterial(encaptable->GetS("optical_gel_material"));
    fParams.optical_gel_surface = Materials::optical_surface[encaptable->GetS("optical_gel_material")];

    fParams.optical_gel_sub_height = 0;  // default to 0
    try {
      fParams.optical_gel_sub_height = encaptable->GetD("optical_gel_sub_height");
    } catch (DBNotFoundError &e) {
    };
    if (fParams.optical_gel_sub_height == 0) {
      info << "No optical gel subtraction value given, set to 0" << newline;
    }

    fParams.pmtposvec.setX(0.0);
    fParams.pmtposvec.setY(0.0);
    fParams.pmtposvec.setZ(0.0);
    try {
      fParams.posvec = encaptable->GetDArray("pmtposoffset");
      fParams.pmtposvec.setX(fParams.posvec[0] * CLHEP::mm);
      fParams.pmtposvec.setY(fParams.posvec[1] * CLHEP::mm);
      fParams.pmtposvec.setZ(fParams.posvec[2] * CLHEP::mm);
    } catch (DBNotFoundError &e) {
    };
  }

  fParams.useSilicaBag = 0;  // default to no silica bag
  try {
    fParams.useSilicaBag = encaptable->GetI("use_silica_bag");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.useSilicaBag == 1) {
    silica_bag_encapsulation_phys = nullptr;
    fParams.silica_bag_material = G4Material::GetMaterial(encaptable->GetS("silica_bag_material"));
    fParams.silica_bag_surface = Materials::optical_surface[encaptable->GetS("silica_bag_material")];
    fParams.silica_bag_dimensions = encaptable->GetDArray("silica_bag_dimensions");
    fParams.silica_bag_position = encaptable->GetDArray("silica_bag_position");
  }

  fParams.useCable = 0;  // default to no cable
  try {
    fParams.useCable = encaptable->GetI("use_cable");
  } catch (DBNotFoundError &e) {
  };
  if (fParams.useCable == 1) {
    cable_encapsulation_phys = nullptr;
    fParams.cable_material = G4Material::GetMaterial(encaptable->GetS("cable_material"));
    fParams.cable_surface = Materials::optical_surface[encaptable->GetS("cable_material")];
    fParams.cable_dimensions = encaptable->GetDArray("cable_dimensions");
    fParams.cable_position = encaptable->GetDArray("cable_position");
  }

  // Encapsulation materials
  fParams.exterior_material = mother->GetMaterial();
  fParams.inner_encapsulation_material = G4Material::GetMaterial(encaptable->GetS("inside_encapsulation_material"));
  fParams.front_encapsulation_material = G4Material::GetMaterial(encaptable->GetS("front_encapsulation_material"));
  fParams.rear_encapsulation_material = G4Material::GetMaterial(encaptable->GetS("rear_encapsulation_material"));

  // Encapsulation surface
  fParams.inner_encapsulation_surface = Materials::optical_surface[encaptable->GetS("inside_encapsulation_material")];
  fParams.front_encapsulation_surface = Materials::optical_surface[encaptable->GetS("front_encapsulation_material")];
  fParams.rear_encapsulation_surface = Materials::optical_surface[encaptable->GetS("rear_encapsulation_material")];
}

G4LogicalVolume *HemisphereEncapsulation::BuildVolume(const std::string &prefix) {
  // ---------- Generate Shapes ----------

  // Generate Envelope which will house encapsulation and PMT
  G4VSolid *envelope_solid = NewEnvelopeSolid("envelope_solid");

  G4VSolid *front_encapsulation_solid =
      new G4Sphere("front_encapsulation_solid",
                   (fParams.encap_radius) * CLHEP::mm,                            // rmin
                   (fParams.encap_radius + fParams.encap_thickness) * CLHEP::mm,  // rmax
                   0.5 * CLHEP::pi, CLHEP::twopi,                                 // phi
                   0.0, 0.5 * CLHEP::pi);                                         // theta

  G4VSolid *rear_encapsulation_solid =
      new G4Sphere("rear_encapsulation_solid",
                   (fParams.encap_radius) * CLHEP::mm,                            // rmin
                   (fParams.encap_radius + fParams.encap_thickness) * CLHEP::mm,  // rmax
                   0.5 * CLHEP::pi, CLHEP::twopi,                                 // phi
                   0.5 * CLHEP::pi, 0.5 * CLHEP::pi);                             // theta

  G4VSolid *dome_flange_solid =
      new G4Tubs("dome_flange_solid",
                 (fParams.encap_radius) * CLHEP::mm,           // rmin
                 (fParams.flange_rmax) * CLHEP::mm,            // rmax
                 (0.5 * fParams.encap_thickness) * CLHEP::mm,  // zhalf, thickness is same as dome thickness so 0.8/2.0
                 0.0, CLHEP::twopi);                           // phi

  front_encapsulation_solid =
      new G4UnionSolid("front_encapsulation_solid", front_encapsulation_solid, dome_flange_solid, 0,
                       G4ThreeVector(0.0, 0.0, (0.5 * fParams.encap_thickness) * CLHEP::mm));

  rear_encapsulation_solid = new G4UnionSolid("rear_encapsulation_solid", rear_encapsulation_solid, dome_flange_solid,
                                              0, G4ThreeVector(0.0, 0.0, -(0.5 * fParams.encap_thickness) * CLHEP::mm));

  G4Tubs *metal_flange_solid = nullptr;
  if (fParams.useMetalFlange == 1) {
    metal_flange_solid = new G4Tubs("front_metal_flange_solid",
                                    (fParams.metal_flange_dimensions[0]) * CLHEP::mm,  // rmin
                                    (fParams.metal_flange_dimensions[1]) * CLHEP::mm,  // rmax
                                    (fParams.metal_flange_dimensions[2]) * CLHEP::mm,  // size z half
                                    0.0, CLHEP::twopi);                                // phi
  }

  G4Sphere *inner_encapsulation_solid = new G4Sphere("inner_encapsulation_solid",
                                                     0.0 * CLHEP::mm,                     // rmin
                                                     (fParams.encap_radius) * CLHEP::mm,  // rmax
                                                     0.0, CLHEP::twopi,                   // phi
                                                     0.0, CLHEP::twopi);                  // theta

  // If optical gel is used generate pmt shape to substract from gel
  // NOTE: THIS WILL ONLY WORK FOR TOROIDAL
  G4VSolid *optical_gel_encapsulation_solid = nullptr;
  GLG4TorusStack *body_solid = nullptr;

  if (fParams.useGel == 1) {
    body_solid = (GLG4TorusStack *)BuildSolid(prefix + "_body_solid");
    if (body_solid != NULL) {
      info << "pmt shape made" << newline;
    }
    optical_gel_encapsulation_solid =
        optical_gel_pmt_subtraction(prefix + "_optical_gel_encapsulation_solid", body_solid);
  }

  G4Box *silica_bag_solid = nullptr;
  if (fParams.useSilicaBag == 1) {
    silica_bag_solid =
        new G4Box("silica_bag_solid", (fParams.silica_bag_dimensions[0]) * CLHEP::mm,
                  (fParams.silica_bag_dimensions[1]) * CLHEP::mm, (fParams.silica_bag_dimensions[2]) * CLHEP::mm);
  }

  G4Tubs *cable_solid = nullptr;
  if (fParams.useCable == 1) {
    cable_solid = new G4Tubs("cable_solid",
                             (fParams.cable_dimensions[0]) * CLHEP::mm,  // rmin
                             (fParams.cable_dimensions[1]) * CLHEP::mm,  // rmax
                             (fParams.cable_dimensions[2]) * CLHEP::mm,  // size z
                             0.0, CLHEP::twopi);                         // phi
  }

  // ---------- Logical volumes ----------

  G4LogicalVolume *envelope_log, *front_encapsulation_log, *rear_encapsulation_log,
      *metal_flange_encapsulation_log = nullptr;
  G4LogicalVolume *inner_encapsulation_log, *optical_gel_encapsulation_log = nullptr,
                                            *silica_bag_encapsulation_log = nullptr, *cable_encapsulation_log = nullptr;

  envelope_log = new G4LogicalVolume(envelope_solid, fParams.exterior_material, "envelope_log");

  front_encapsulation_log =
      new G4LogicalVolume(front_encapsulation_solid, fParams.front_encapsulation_material, "front_encapsulation_log");

  rear_encapsulation_log =
      new G4LogicalVolume(rear_encapsulation_solid, fParams.rear_encapsulation_material, "rear_encapsulation_log");

  if (fParams.useMetalFlange == 1) {
    metal_flange_encapsulation_log =
        new G4LogicalVolume(metal_flange_solid, fParams.metal_flange_material, "metal_flange_encapsulation_log");
  }

  inner_encapsulation_log =
      new G4LogicalVolume(inner_encapsulation_solid, fParams.inner_encapsulation_material, "inner_encapsulation_log");

  if (fParams.useGel == 1) {
    optical_gel_encapsulation_log = new G4LogicalVolume(optical_gel_encapsulation_solid, fParams.optical_gel_material,
                                                        "optical_gel_encapsulation_log");
  }

  if (fParams.useSilicaBag == 1) {
    silica_bag_encapsulation_log =
        new G4LogicalVolume(silica_bag_solid, fParams.silica_bag_material, "silica_bag_encapsulation_log");
  }

  if (fParams.useCable == 1) {
    cable_encapsulation_log = new G4LogicalVolume(cable_solid, fParams.cable_material, "cable_encapsulation_log");
  }

  // ---------- Physical volumes ----------

  front_encapsulation_phys =
      new G4PVPlacement(0,                                          // rotation
                        G4ThreeVector(0.0, 0.0, 0.0),               // position
                        front_encapsulation_log,                    // the logical volume
                        prefix + "_front_dome_encapsulation_phys",  // a name for this physical volume
                        envelope_log,                               // the mother volume
                        false,                                      // no boolean ops
                        0);                                         // copy number

  rear_encapsulation_phys =
      new G4PVPlacement(0,                                         // rotation
                        G4ThreeVector(0.0, 0.0, 0.0),              // position
                        rear_encapsulation_log,                    // the logical volume
                        prefix + "_rear_dome_encapsulation_phys",  // a name for this physical volume
                        envelope_log,                              // the mother volume
                        false,                                     // no boolean ops
                        0);                                        // copy number

  if (fParams.useMetalFlange == 1) {
    double metal_flange_zpos = (fParams.encap_thickness + fParams.metal_flange_dimensions[2]) * CLHEP::mm;
    front_metal_encapsulaion_flange_phys =
        new G4PVPlacement(0,                                                  // rotation
                          G4ThreeVector(0.0, 0.0, metal_flange_zpos),         // position
                          metal_flange_encapsulation_log,                     // the logical volume
                          prefix + "_front_metal_flange_encapsulation_phys",  // a name for this physical volume
                          envelope_log,                                       // the mother volume
                          false,                                              // no boolean ops
                          0);                                                 // copy number

    rear_metal_encapsulation_flange_phys =
        new G4PVPlacement(0,                                                // rotation
                          G4ThreeVector(0.0, 0.0, -metal_flange_zpos),      // position
                          metal_flange_encapsulation_log,                   // the logical volume
                          prefix + "rear_metal_flange_encapsulation_phys",  // a name for this physical volume
                          envelope_log,                                     // the mother volume
                          false,                                            // no boolean ops
                          0);                                               // copy number
  }

  inner_encapsulation_phys =
      new G4PVPlacement(0,                                            // rotation
                        G4ThreeVector(0.0, 0.0, 0.0),                 // position
                        inner_encapsulation_log,                      // the logical volume
                        prefix + "_inner_volume_encapsulation_phys",  // a name for this physical volume
                        envelope_log,                                 // the mother volume
                        false,                                        // no boolean ops
                        0);

  if (fParams.useGel == 1) {
    optical_gel_encapsulation_phys =
        new G4PVPlacement(0,                                   // rotation
                          G4ThreeVector(0.0, 0.0, 0.0),        // position
                          optical_gel_encapsulation_log,       // the logical volume
                          prefix + "_gel_encapsulation_phys",  // a name for this physical volume
                          inner_encapsulation_log,             // the mother volume
                          false,                               // no boolean ops
                          0);
  }

  if (fParams.useSilicaBag == 1) {
    G4ThreeVector silicaposition(fParams.silica_bag_position[0] * CLHEP::mm, fParams.silica_bag_position[1] * CLHEP::mm,
                                 fParams.silica_bag_position[2] * CLHEP::mm);

    silica_bag_encapsulation_phys =
        new G4PVPlacement(0,                                      // rotation
                          silicaposition,                         // position
                          silica_bag_encapsulation_log,           // the logical volume
                          prefix + "_silica_encapsulation_phys",  // a name for this physical volume
                          inner_encapsulation_log,                // the mother volume
                          false,                                  // no boolean ops
                          0);
  }

  if (fParams.useCable == 1) {
    G4ThreeVector cableposition(fParams.cable_position[0] * CLHEP::mm, fParams.cable_position[1] * CLHEP::mm,
                                fParams.cable_position[2] * CLHEP::mm);

    cable_encapsulation_phys =
        new G4PVPlacement(0,                                     // rotation
                          cableposition,                         // position
                          cable_encapsulation_log,               // the logical volume
                          prefix + "_cable_encapsulation_phys",  // a name for this physical volume
                          inner_encapsulation_log,               // the mother volume
                          false,                                 // no boolean ops
                          0);
  }

  // ---------- Skin surfaces ----------
  new G4LogicalSkinSurface("front_encapsulation_skin",
                           front_encapsulation_log,               // Logical Volume
                           fParams.front_encapsulation_surface);  // Surface Property

  new G4LogicalSkinSurface("rear_encapsulation_skin",
                           rear_encapsulation_log,               // Logical Volume
                           fParams.rear_encapsulation_surface);  // Surface Property

  if (fParams.useMetalFlange == 1) {
    new G4LogicalSkinSurface("metal_flange_encapsulation_skin",
                             metal_flange_encapsulation_log,  // Logical Volume
                             fParams.metal_flange_surface);   // Surface Property
  }

  new G4LogicalSkinSurface("inner_encapsulation_skin",
                           inner_encapsulation_log,               // Logical Volume
                           fParams.inner_encapsulation_surface);  // Surface Property

  if (fParams.useGel == 1) {
    new G4LogicalSkinSurface("optical_gel_encapsulation_skin",
                             optical_gel_encapsulation_log,  // Logical Volume
                             fParams.optical_gel_surface);   // Surface Property
  }

  if (fParams.useSilicaBag == 1) {
    new G4LogicalSkinSurface("silica_bag_encapsulation_skin",
                             silica_bag_encapsulation_log,  // Logical Volume
                             fParams.silica_bag_surface);   // Surface Property
  }

  if (fParams.useCable == 1) {
    new G4LogicalSkinSurface("cable_encapsulation_skin",
                             cable_encapsulation_log,  // Logical Volume
                             fParams.cable_surface);   // Surface Property
  }

  // ---------- Border Surface ----------

  new G4LogicalBorderSurface(prefix + "_backencap_logsurf1", rear_encapsulation_phys, inner_encapsulation_phys,
                             fParams.rear_encapsulation_surface);
  new G4LogicalBorderSurface(prefix + "_backencap_logsurf2", inner_encapsulation_phys, rear_encapsulation_phys,
                             fParams.rear_encapsulation_surface);

  // ---------- Visual Attributes ----------
  G4VisAttributes *visAtt;

  envelope_log->SetVisAttributes(G4VisAttributes::GetInvisible());

  visAtt = new G4VisAttributes(G4Color(0.44, 0.52, 0.78, 0.5));
  front_encapsulation_log->SetVisAttributes(visAtt);

  visAtt = new G4VisAttributes(G4Color(0., 0.03, 0.14, 1.));
  rear_encapsulation_log->SetVisAttributes(visAtt);

  if (fParams.useMetalFlange == 1) {
    visAtt = new G4VisAttributes(G4Color(0.68, 0.68, 0.68, 1.0));
    metal_flange_encapsulation_log->SetVisAttributes(visAtt);
  }

  visAtt = new G4VisAttributes(G4Color(0.77, 0.97, 1., 0.3));
  inner_encapsulation_log->SetVisAttributes(visAtt);

  if (fParams.useGel == 1) {
    visAtt = new G4VisAttributes(G4Color(0.87, 0.81, 0.11, 0.5));
    optical_gel_encapsulation_log->SetVisAttributes(visAtt);
  }

  if (fParams.useSilicaBag == 1) {
    visAtt = new G4VisAttributes(G4Color(0.3, 0.1, 0.1, 1.0));
    silica_bag_encapsulation_log->SetVisAttributes(visAtt);
  }
  if (fParams.useCable == 1) {
    visAtt = new G4VisAttributes(G4Color(1.0, 0.18, 0.8, 1.0));
    cable_encapsulation_log->SetVisAttributes(visAtt);
  }

  return envelope_log;
}

G4VSolid *HemisphereEncapsulation::BuildSolid(const std::string &_name) {
  GLG4TorusStack *body = new GLG4TorusStack(_name);
  body->SetAllParameters(fParams.zOrigin.size(), &fParams.zEdge[0], &fParams.rhoEdge[0], &fParams.zOrigin[0]);
  return body;
}

G4VSolid *HemisphereEncapsulation::optical_gel_height_subtraction(const std::string &_name) {
  G4Sphere *optical_gel_1 =
      new G4Sphere("optical_gel_1_encapsulation_solid", 0.0 * CLHEP::mm, (fParams.encap_radius) * CLHEP::mm,
                   0.5 * CLHEP::pi, CLHEP::twopi, 0.0, 0.5 * CLHEP::pi);

  G4Tubs *gel_subtract = new G4Tubs("gel_sub_solid", 0.0,
                                    (fParams.encap_radius) * CLHEP::mm,            // solid cylinder
                                    (fParams.optical_gel_sub_height) * CLHEP::mm,  // half height of cylinder
                                    0.0, CLHEP::twopi);                            // cylinder complete in phi

  return new G4SubtractionSolid(_name, optical_gel_1, gel_subtract, 0, G4ThreeVector(0.0, 0.0, 0.0));
}

G4VSolid *HemisphereEncapsulation::optical_gel_pmt_subtraction(const std::string &_name, GLG4TorusStack *body) {
  G4VSolid *optical_gel_2 = optical_gel_height_subtraction("temp_gel" + _name);

  return new G4SubtractionSolid(_name, optical_gel_2, body, 0, fParams.pmtposvec);
}

G4PVPlacement *HemisphereEncapsulation::PlaceEncap(G4RotationMatrix *pmtrot, G4ThreeVector pmtpos,
                                                   const std::string &_name, G4LogicalVolume *logi_pmt,
                                                   G4VPhysicalVolume *mother_phys, bool booleanSolid, int copyNo) {
  return new G4PVPlacement(pmtrot, pmtpos, _name, logi_pmt, mother_phys, booleanSolid, copyNo);
}

G4VSolid *HemisphereEncapsulation::NewEnvelopeSolid(const std::string &_name) {
  G4Sphere *outer_s = new G4Sphere(_name + "_main", 0. * CLHEP::mm, fParams.envelope_radius * CLHEP::mm, 0.0,
                                   CLHEP::twopi, 0.0, CLHEP::twopi);
  return outer_s;
}

}  // namespace RAT
