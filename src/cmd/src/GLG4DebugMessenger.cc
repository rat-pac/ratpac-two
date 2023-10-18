// This file is part of the GenericLAND software library.
// $Id: GLG4DebugMessenger.cc,v 1.2 2005/09/15 21:57:04 sekula Exp $
//
// GLG4DebugMessenger.cc by Glenn Horton-Smith, Dec 1999
////////////////////////////////////////////////////////////////
// GLG4DebugMessenger
////////////////////////////////////////////////////////////////

#include <RAT/DB.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/GLG4DebugMessenger.hh>
#include <RAT/Log.hh>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "G4HadronElasticProcess.hh"
#include "G4HadronInelasticProcess.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4ProcessTable.hh"
#include "G4RunManager.hh"
#include "G4SolidStore.hh"
#include "G4Timer.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ios.hh"
#include "RAT/GLG4TestSolid.hh"
#include "Randomize.hh"
#include "globals.hh"

GLG4DebugMessenger::GLG4DebugMessenger(RAT::DetectorConstruction *mydetector) : myDetector(mydetector) {
  // the glg4debug directory
  G4UIdirectory *DebugDir = new G4UIdirectory("/glg4debug/");
  DebugDir->SetGuidance("User-added debugging, tests, and diagnostics.");

  // the dumpmat command
  DumpMaterialsCmd = new G4UIcommand("/glg4debug/dumpmat", this);
  DumpMaterialsCmd->SetGuidance("Dump entire materials table or one material's properties table");

  G4UIparameter *aParam;
  aParam = new G4UIparameter("material", 's', true);  // omittable
  DumpMaterialsCmd->SetParameter(aParam);

  // the dumpgeom command
  DumpGeomCmd = new G4UIcommand("/glg4debug/dumpgeom", this);
  DumpGeomCmd->SetGuidance("Dump the geometry RAT::information for the entire detector");
  aParam = new G4UIparameter("physicalVolume", 's', true);  // omittable
  DumpGeomCmd->SetParameter(aParam);

  // the testsolid command
  TestSolidCmd = new G4UIcommand("/glg4debug/testsolid", this);
  TestSolidCmd->SetGuidance("Test a solid using GLG4TestSolid routines");
  aParam = new G4UIparameter("nloop", 'i', true);  // omittable
  TestSolidCmd->SetParameter(aParam);
  aParam = new G4UIparameter("solid name", 's', true);  // omittable
  TestSolidCmd->SetParameter(aParam);

  // the setmaterial commmand
  G4UIcommand *cmd = new G4UIcommand("/glg4debug/setmaterial", this);
  cmd->SetGuidance("Change the material in a region");
  cmd->SetParameter(new G4UIparameter("logicalVolume", 's', false));
  cmd->SetParameter(new G4UIparameter("material", 's', false));

  // the database override commmand
  cmd = new G4UIcommand("/glg4debug/glg4param", this);
  cmd->SetGuidance("Inspect or modify the options and values database");
  cmd->SetParameter(new G4UIparameter("identifier", 's', false));
  cmd->SetParameter(new G4UIparameter("value", 'd', true));

  // the dumpelem command
  cmd = new G4UIcommand("/glg4debug/dumpelem", this);
  cmd->SetGuidance("Dump entire element table or one element's properties");
  cmd->SetParameter(new G4UIparameter("name", 's', true));

  // the setseed command
  cmd = new G4UIcommand("/glg4debug/setseed", this);
  cmd->SetGuidance("Change random number generator state using setSeed(seed, luxury).");
  cmd->SetGuidance(
      "The \"luxury\" parameter may be omitted; the default random number\n"
      "generator doesn't use it anyway.\n"
      "There is no getseed command, for Very Good Reasons, so use Geant4's\n"
      "/run/storeRandomNumberStatus and restoreRandomNumberStatus commands\n"
      "for saving/restoring the generator status in general.");
  cmd->SetParameter(new G4UIparameter("seed", 'd', false));
  cmd->SetParameter(new G4UIparameter("luxury_level", 'd', true));

  // the SetRunIDCounter command
  cmd = new G4UIcommand("/glg4debug/SetRunIDCounter", this);
  cmd->SetGuidance("Set Geant4 run number for next run");
  cmd->SetParameter(new G4UIparameter("number", 'i', false));

#ifdef G4DEBUG
  // illuminationMap
  cmd = new G4UIcommand("/glg4debug/dump_illumination_map", this);
  cmd->SetGuidance("Dump a pretty picture of particle hits, for debugging");
#endif
}

GLG4DebugMessenger::~GLG4DebugMessenger() {}

static void DumpGeom(G4VPhysicalVolume *pv, const char *s) {
  RAT::info << "*******************************" << newline;
  RAT::info << "Physical volume dump for " << s << newline;
  RAT::info << " Name: " << pv->GetName() << newline;

  G4LogicalVolume *lv = pv->GetLogicalVolume();
  RAT::info << " Logical volume name: " << lv->GetName() << newline;
  RAT::info << " Solid name: " << lv->GetSolid()->GetName() << newline;

  G4Material *m = lv->GetMaterial();
  RAT::info << " Material: " << m->GetName() << newline;
  G4cout << (*m) << newline;

  ///  G4PhysicalVolume::GetMother() was removed in Geant4 version 06.
  // G4VPhysicalVolume* mother= pv->GetMother();
  // if ( mother )
  //   RAT::info << "Mother volume is " << mother->GetName() << newline;
  // else
  //   RAT::info << "Has no mother!" << newline;

  G4int ndaught = lv->GetNoDaughters();
  if (ndaught == 0) {
    RAT::info << "Has no daughters." << newline;
  } else {
    RAT::info << "Has " << ndaught << " daughters:" << newline;
    for (G4int i = 0; i < ndaught; i++) RAT::info << "\t" << lv->GetDaughter(i)->GetName();
    RAT::info << newline;
  }
}

static void SetMaterial(G4String newValues) {
  // parse out names
  std::istringstream iss(newValues.c_str());
  G4String lvName;
  G4String matName;
  iss >> lvName >> matName;
  if (iss.fail()) {
    RAT::warn << "Could not parse volume and material name from command args" << newline;
    return;
  }

  // access the store of logical volumes
  G4LogicalVolumeStore *lvs = G4LogicalVolumeStore::GetInstance();
  G4LogicalVolume *lv = NULL;
  G4int nlv = lvs->size();
  G4int ilv;
  for (ilv = 0; ilv < nlv; ilv++) {
    lv = (*lvs)[ilv];
    if (!lv) break;
    if (lv->GetName() == lvName) break;
  }
  if (lv == NULL || ilv >= nlv) {  // not found
    RAT::warn << "Error, logical volume named \'" << lvName << "\' not found" << newline;
    return;
  }

  // access the store of materials
  G4Material *mat = G4Material::GetMaterial(matName);
  if (mat == NULL) {
    RAT::warn << "Error, material named \'" << matName << "\' not found" << newline;
    return;
  }

  // set the material
  lv->SetMaterial(mat);
  RAT::info << "Set material of " << lv->GetName() << " to " << mat->GetName() << newline;
}

void GLG4DebugMessenger::SetNewValue(G4UIcommand *command, G4String newValues) {
  // DumpMaterialsCmd
  if (command == DumpMaterialsCmd) {
    if (newValues == "") {
      G4cout << *(G4Material::GetMaterialTable()) << newline;
    } else {
      G4Material *m = G4Material::GetMaterial(newValues);
      if (m == NULL) {
        RAT::warn << "Unknown material " << newValues << newline;
        return;
      }
      G4cout << (*m) << newline;
      G4MaterialPropertiesTable *mpt = m->GetMaterialPropertiesTable();
      if (mpt == NULL) {
        RAT::info << "This material has no material properties table." << newline;
      } else {
        mpt->DumpTable();
      }
    }
  }

  // DumpGeomCmd
  else if (command == DumpGeomCmd) {
    if (newValues == "") {
      DumpGeom(myDetector->GetWorld(), "myDetector->GetWorld()");
    } else {
      // access the store of physical volumes
      G4PhysicalVolumeStore *pvs = G4PhysicalVolumeStore::GetInstance();
      G4VPhysicalVolume *pv = NULL;
      G4int npv = pvs->size();
      G4int ipv;
      for (ipv = 0; ipv < npv; ipv++) {
        pv = (*pvs)[ipv];
        if (!pv) break;
        if (pv->GetName() == newValues) break;
      }
      if (pv == NULL || ipv >= npv) {  // not found
        RAT::warn << "Error, name \'" << newValues << "\' not found" << newline;
      } else {
        DumpGeom(pv, newValues);
      }
    }
  }

  // TestSolidCmd
  else if (command == TestSolidCmd) {
    // parse out nloop and name
    char *nameptr = NULL;
    G4int nloop = strtol((const char *)newValues, &nameptr, 0);
    if (nameptr == NULL) nameptr = (char *)(const char *)newValues;
    while (*nameptr == ' ') nameptr++;

    // access the store of solids
    G4SolidStore *theSolidStore = G4SolidStore::GetInstance();
    G4int nsolid = theSolidStore->size();
    G4int isolid;

    // if no arguments or nloop==0, list the solids
    if (nloop == 0) {
      RAT::info << "Here is a list of solid names" << newline;
      for (isolid = 0; isolid < nsolid; isolid++) {
        G4VSolid *aSolid = (*theSolidStore)[isolid];
        if (!aSolid) break;
        RAT::info << aSolid->GetName() << newline;
      }
    }
    // if arguments given, find the named solid and test it
    else {
      G4VSolid *aSolid = NULL;
      for (isolid = 0; isolid < nsolid; isolid++) {  // scan for solid
        aSolid = (*theSolidStore)[isolid];
        if (!aSolid) break;
        if (aSolid->GetName() == nameptr) break;
      }
      if (aSolid == NULL || isolid >= nsolid) {  // solid not found
        RAT::warn << "Error, solid name \'" << nameptr << "\' not found" << newline;
        RAT::warn << "Enter command with no parameters to get list of names" << newline;
      } else {  // we found the solid, now test it
        G4Timer timer;
        RAT::info << "Testing " << nameptr << " with " << nloop << " loops " << newline;
        timer.Start();
        RAT::info << GLG4TestSolid::Test(*((*theSolidStore)[isolid]), nloop) << newline;
        timer.Stop();
        RAT::info << "Elapsed time: User=" << timer.GetUserElapsed() << " System=" << timer.GetSystemElapsed()
                  << " Real=" << timer.GetRealElapsed() << newline;
      }
    }
  }

  else if (command->GetCommandName() == "setmaterial") {
    SetMaterial(newValues);
  }

  else if (command->GetCommandName() == "glg4param") {
    std::istringstream iss(newValues.c_str());
    G4String parameterName;
    G4double new_value;
    iss >> parameterName;
    if (iss.fail()) {
      RAT::warn << "Could not parse parameter name from command args" << newline;
      return;
    }
    iss >> new_value;
    // set new value, if value was provided
    if (!(iss.fail())) {
      RAT::DB *db = RAT::DB::Get();
      int value = new_value != 0.0 ? 0 : 1;  // invert meaning

      if (parameterName == "omit_muon_processes") {
        db->Set("MC", "", "muon_processes", value);
      } else if (parameterName == "omit_hadronic_processes") {
        db->Set("MC", "", "hadronic_processes", value);
      } else {
        RAT::Log::Die(
            dformat("/glg4debug/glg4param has been deprecated and '%s' is unsupported", parameterName.c_str()));
      }
    }
  } else if (command->GetCommandName() == "dumpelem") {
    if (newValues == "") {
      G4cout << *(G4Element::GetElementTable()) << newline;
    } else {
      G4cout << *(G4Element::GetElement(newValues)) << newline;
    }
  } else if (command->GetCommandName() == "setseed") {
    std::istringstream iss(newValues.c_str());
    long seed = 1;
    int luxury_level = 0;
    iss >> seed >> luxury_level;
    CLHEP::HepRandom::getTheEngine()->setSeed(seed, luxury_level);
  } else if (command->GetCommandName() == "SetRunIDCounter") {
    int i = atoi(newValues);
    G4RunManager::GetRunManager()->SetRunIDCounter(i);
    RAT::info << "Set RunIDCounter to " << i << newline;
  }
#ifdef G4DEBUG
  else if (command->GetCommandName() == "dump_illumination_map") {
    extern int GLG4SteppingAction_dump_IlluminationMap(void);
    GLG4SteppingAction_dump_IlluminationMap();
  }
#endif

  // invalid command
  else {
    RAT::warn << "invalid GLG4 \"set\" command" << newline;
  }
}

G4String GLG4DebugMessenger::GetCurrentValue(G4UIcommand * /*command*/) {
  return G4String("invalid GLG4DebugMessenger \"get\" command");
}
