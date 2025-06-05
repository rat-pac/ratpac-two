// This is the encapsulation model that is used for BUTTON.
// The 96 Hamamatsu r7081pe PMTs are encapsulated by two acryilic domes that are held together with metal flanges.
// Created by Lewis Sexton (Sheffield) and Adam Tarrant (Liverpool)

#ifndef __RAT_HemisphereEncapsulation__
#define __RAT_HemisphereEncapsulation__

#include <G4Box.hh>
#include <G4GenericPolycone.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4PVPlacement.hh>
#include <G4Paraboloid.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>
#include <G4VSensitiveDetector.hh>
#include <G4VSolid.hh>
#include <G4VisAttributes.hh>
#include <RAT/DB.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4TorusStack.hh>
#include <RAT/PMTEncapsulation.hh>
#include <string>
#include <vector>
namespace RAT {

struct HemisphereEncapsulationParams {
  int useMetalFlange;
  int useGel;
  int useSilicaBag;
  int useCable;

  // encapsulation object dimensions
  double envelope_radius;
  double encap_radius;
  double encap_thickness;
  double flange_rmax;
  double dome_flange_thickness;
  double metal_flange_thickness;
  double optical_gel_sub_height;
  std::vector<double> metal_flange_dimensions;
  std::vector<double> silica_bag_dimensions;
  std::vector<double> silica_bag_position;
  std::vector<double> cable_dimensions;
  std::vector<double> cable_position;

  // PMT Body
  std::vector<double> zEdge;    // n+1
  std::vector<double> rhoEdge;  // n+1
  std::vector<double> zOrigin;  // n
  G4ThreeVector pmtposvec;
  std::vector<double> posvec;

  G4Material *exterior_material;
  G4Material *inner_encapsulation_material;
  G4Material *front_encapsulation_material;
  G4Material *rear_encapsulation_material;
  G4Material *metal_flange_material;
  G4Material *silica_bag_material;
  G4Material *cable_material;
  G4Material *optical_gel_material;

  G4OpticalSurface *inner_encapsulation_surface;
  G4OpticalSurface *front_encapsulation_surface;
  G4OpticalSurface *rear_encapsulation_surface;
  G4OpticalSurface *metal_flange_surface;
  G4OpticalSurface *cable_surface;
  G4OpticalSurface *silica_bag_surface;
  G4OpticalSurface *optical_gel_surface;
};

class HemisphereEncapsulation : public PMTEncapsulation {
 public:
  HemisphereEncapsulation(DBLinkPtr encaptable, DBLinkPtr pmttable, G4LogicalVolume *mother);
  virtual ~HemisphereEncapsulation() {}

  virtual G4LogicalVolume *BuildVolume(const std::string &prefix);
  virtual G4VSolid *BuildSolid(const std::string &prefix);
  virtual G4PVPlacement *PlaceEncap(G4RotationMatrix *pmtrot, G4ThreeVector pmtpos, const std::string &name,
                                    G4LogicalVolume *logi_pmt, G4VPhysicalVolume *mother_phys, bool booleanSolid,
                                    int copyNo);

 protected:
  G4VSolid *NewEnvelopeSolid(const std::string &name);
  G4VSolid *NewEncapsulationSolid(const std::string &name);
  G4VSolid *optical_gel_height_subtraction(const std::string &_name);
  G4VSolid *optical_gel_pmt_subtraction(const std::string &_name, GLG4TorusStack *body);

  // phyiscal volumes
  G4PVPlacement *inner_encapsulation_phys;
  G4PVPlacement *optical_gel_encapsulation_phys;
  G4PVPlacement *front_encapsulation_phys;
  G4PVPlacement *rear_encapsulation_phys;
  G4PVPlacement *front_metal_encapsulaion_flange_phys;
  G4PVPlacement *rear_metal_encapsulation_flange_phys;
  G4PVPlacement *silica_bag_encapsulation_phys;
  G4PVPlacement *cable_encapsulation_phys;

  HemisphereEncapsulationParams fParams;
};

}  // namespace RAT

#endif
