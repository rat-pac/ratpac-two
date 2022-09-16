#include <TVector3.h>

#include <G4GeometryManager.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4SDManager.hh>
#include <G4SolidStore.hh>
#include <G4VPhysicalVolume.hh>
#include <RAT/BWVetGenericChamber.hh>
#include <RAT/DB.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/DetectorFactory.hh>
#include <RAT/GeoBuilder.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <RAT/PhotonThinning.hh>
#include <RAT/Rat.hh>
#include <string>

namespace RAT {

DetectorConstruction *DetectorConstruction::sDetectorConstruction = NULL;

DetectorConstruction::DetectorConstruction() {
}

G4VPhysicalVolume *DetectorConstruction::Construct() {
  // Load the DETECTOR table
  DB *db = DB::Get();
  DBLinkPtr ldetector = db->GetLink("DETECTOR");
  std::string experiment = "";

  // Load experiment RATDB files before doing anything else
  try {
    experiment = ldetector->GetS("experiment");
    info << "Loading experiment-specific RATDB files for: " << experiment << newline;
    for (auto dir : Rat::ratdb_directories) {
      std::string experimentDirectoryString = dir + "/" + experiment;
      int result = db->LoadAll(experimentDirectoryString);
      if (result == 2) {
        info << "Found experiment files in " << experimentDirectoryString << newline;
        break;
      }
    }
  } catch (DBNotFoundError &e) {
    info << "No experiment-specific tables loaded." << newline;
  }

  try {
    std::string detector_factory = ldetector->GetS("detector_factory");
    info << "Loading detector factory " << detector_factory << newline;
    DetectorFactory::DefineWithFactory(detector_factory, ldetector);
  } catch (DBNotFoundError &e) {
    info << "DetectorConstruction: could not access " << e.table << "[" << e.index << "]." << e.field << newline;
    try {
      std::string geo_file = ldetector->GetS("geo_file");
      info << "Loading detector geometry from " << geo_file << newline;
      if (db->Load(geo_file) == 0) {
        Log::Die("DetectorConstruction: Could not open detector geometry");
      }
    } catch (DBNotFoundError &_e) {
      info << "DetectorConstruction: could not access " << _e.table << "[" << _e.index << "]." << _e.field << newline;
      Log::Die("DetectorConstruction: Could not open geo_file or detector_factory");
    }
  }

  info << "Constructing detector materials...\n";
  ConstructMaterials();

  // Delete the old detector if we are constructing a new one
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();

  // Add sensitive volumes here (only veto for now)
  G4SDManager *sdman = G4SDManager::GetSDMpointer();
  G4VSensitiveDetector *veto = new BWVetGenericChamber("/mydet/veto/genericchamber");
  sdman->AddNewDetector(veto);

  // Setup photon thinning parameters
  PhotonThinning::Init();

  GeoBuilder geo;
  fWorldPhys = geo.ConstructAll();

  return fWorldPhys;
}

void DetectorConstruction::ConstructMaterials() {
  Materials::ConstructMaterials();
  Materials::LoadOpticalSurfaces();
}

DetectorConstruction *DetectorConstruction::GetDetectorConstruction() {
  if (!sDetectorConstruction) {
    sDetectorConstruction = new DetectorConstruction();
  }
  return sDetectorConstruction;
}

}  // namespace RAT
