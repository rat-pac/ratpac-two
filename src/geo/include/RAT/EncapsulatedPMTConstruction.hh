// Adam T: A encapsulat// Adam T: A encapsulated pmt based off the Toroidal pmt construction model for
// button (some of the offesets will need adjusted for different pmt this will
// work for the r7081pe model)
#ifndef __RAT_EncapsulatedPMTConstruction__
#define __RAT_EncapsulatedPMTConstruction__

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4PVPlacement.hh>
#include <G4VSensitiveDetector.hh>
#include <G4VSolid.hh>
#include <RAT/DB.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4TorusStack.hh>
#include <RAT/PMTConstruction.hh>
#include <RAT/WaveguideFactory.hh>
#include <string>
#include <vector>

#include <G4Box.hh>
#include <G4GenericPolycone.hh>
#include <G4Paraboloid.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>
#include <G4VisAttributes.hh>
namespace RAT {

struct EncapsulatedPMTConstructionParams {
  EncapsulatedPMTConstructionParams() {
    efficiencyCorrection = 1.0;
    simpleVis = false;
    photocathode_MINrho = 0.0;
    photocathode_MAXrho = 0.0;
  };

  bool simpleVis;

  // Envelope control
  bool useEnvelope;
  double faceGap;
  double minEnvelopeRadius;

  // Body
  std::vector<double> zEdge;   // n+1
  std::vector<double> rhoEdge; // n+1
  std::vector<double> zOrigin; // n
  double wallThickness;        // mm

  double dynodeRadius;        // mm
  double dynodeTop;           // mm
  double photocathode_MINrho; // mm
  double photocathode_MAXrho; // mm

  G4Material *exterior;
  G4Material *glass;
  G4Material *vacuum;
  G4Material *dynode;

  G4OpticalSurface *photocathode;
  G4OpticalSurface *mirror;
  G4OpticalSurface *dynode_surface;

  // Encapsulation
  G4Material *in_encapsulation_material;
  G4Material *front_encapsulation_material;
  G4Material *rear_encapsulation_material;
  G4Material *metal_flange_material;
  G4Material *acrylic_flange_material;
  G4Material *silica_bag_material;
  G4Material *cable_material;
  G4Material *optical_gel_material;

  G4OpticalSurface *in_encapsulation_surface;
  G4OpticalSurface *front_encapsulation_surface;
  G4OpticalSurface *rear_encapsulation_surface;
  G4OpticalSurface *metal_flange_surface;
  G4OpticalSurface *acrylic_flange_surface;
  G4OpticalSurface *cable_surface;
  G4OpticalSurface *silica_bag_surface;
  G4OpticalSurface *optical_gel_surface;

  double efficiencyCorrection; // default to 1.0 for no correction
};

// Construction for PMTs based on GLG4TorusStack
class EncapsulatedPMTConstruction : public PMTConstruction {
public:
  EncapsulatedPMTConstruction(DBLinkPtr params, G4LogicalVolume *mother);
  virtual ~EncapsulatedPMTConstruction() {}

  virtual G4LogicalVolume *BuildVolume(const std::string &prefix);
  virtual G4VSolid *BuildSolid(const std::string &prefix);
  virtual G4PVPlacement *PlacePMT(G4RotationMatrix *pmtrot,
                                  G4ThreeVector pmtpos, const std::string &name,
                                  G4LogicalVolume *logi_pmt,
                                  G4VPhysicalVolume *mother_phys,
                                  bool booleanSolid, int copyNo);

protected:
  G4VSolid *NewEnvelopeSolid(const std::string &name);
  G4VSolid *NewEncapsulationSolid(const std::string &name);
  G4VSolid *optical_gel_height_subtraction(const std::string &_name);  
  G4VSolid *optical_gel_pmt_subtraction(const std::string &_name, GLG4TorusStack *body);
  void CalcInnerParams(GLG4TorusStack *body, std::vector<double> &innerZEdge,
                       std::vector<double> &innerRhoEdge, int &equatorIndex,
                       double &zLowestDynode);

  // phyiscal volumes
  G4PVPlacement *envelope_phys;
  G4PVPlacement *body_phys;
  G4PVPlacement *inner1_phys;
  G4PVPlacement *inner2_phys;
  G4PVPlacement *central_gap_phys;
  G4PVPlacement *dynode_phys;

  ///// Encapsulation
  G4PVPlacement *in_encapsulation_phys;
  G4PVPlacement *optical_gel_encapsulation_phys;
  G4PVPlacement *encapsulation_phys;
  G4PVPlacement *front_encapsulation_phys;
  G4PVPlacement *rear_encapsulation_phys = 0;
  G4PVPlacement *front_metal_encapsulaion_flange_phys;
  G4PVPlacement *rear_metal_encapsulation_flange_phys;
  G4PVPlacement *acrylic_flange_encapsulaion_phys;
  G4PVPlacement *silica_bag_encapsulation_phys;
  G4PVPlacement *silica_bag_encapsulation_phys2;
  G4PVPlacement *cable_encapsulation_phys;

  G4LogicalVolume *log_pmt;

  WaveguideFactory *fWaveguideFactory;

  EncapsulatedPMTConstructionParams fParams;
};

} // namespace RAT

#endif
