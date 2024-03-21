#ifndef GDMLParser_hh
#define GDMLParser_hh

#include "G4GDMLEvaluator.hh"
#include "G4GDMLReadStructure.hh"
#include "G4Navigator.hh"
#include "G4STRead.hh"
#include "G4Threading.hh"
#include "G4TransportationManager.hh"
#include "RAT/GDMLMessenger.hh"
#include "RAT/GDMLWriteStructure.hh"

#define G4GDML_DEFAULT_SCHEMALOCATION                                 \
  G4String(                                                           \
      "http://service-spi.web.cern.ch/service-spi/app/releases/GDML/" \
      "schema/gdml.xsd")

namespace RAT {
class GDMLParser {
 public:
  GDMLParser();
  GDMLParser(G4GDMLReadStructure*);
  GDMLParser(G4GDMLReadStructure*, GDMLWriteStructure*);
  ~GDMLParser();
  //
  // Parser constructors & destructor

  inline void Read(const G4String& filename, G4bool Validate = true);
  //
  // Imports geometry with world-volume, specified by the GDML filename
  // in input. Validation against schema is activated by default.

  inline void ReadModule(const G4String& filename, G4bool Validate = true);
  //
  // Imports a single GDML module, specified by the GDML filename
  // in input. Validation against schema is activated by default.

  inline void Write(const G4String& filename, const G4VPhysicalVolume* pvol = 0, G4bool storeReferences = true,
                    const G4String& SchemaLocation = G4GDML_DEFAULT_SCHEMALOCATION);
  //
  // Exports on a GDML file, specified by 'filename' a geometry tree
  // starting from 'pvol' as top volume. Uniqueness of stored entities
  // is guaranteed by storing pointer-references by default.
  // Alternative path for the schema location can be specified; by default
  // the URL to the GDML web site is used.

  inline void Write(const G4String& filename, const G4LogicalVolume* lvol, G4bool storeReferences = true,
                    const G4String& SchemaLocation = G4GDML_DEFAULT_SCHEMALOCATION);
  //
  // Exports on a GDML file, specified by 'filename' a geometry tree
  // starting from 'pvol' as top volume. Uniqueness of stored entities
  // is guaranteed by storing pointer-references by default.
  // Alternative path for the schema location can be specified; by default
  // the URL to the GDML web site is used. Same as method above except
  // that the logical volume must be provided here.

  inline G4LogicalVolume* ParseST(const G4String& name, G4Material* medium, G4Material* solid);
  //
  // Imports a tessellated geometry stored as STEP-Tools files
  // 'name.geom' and 'name.tree'. It returns a pointer of a generated
  // mother volume with 'medium' material associated, including the
  // imported tessellated geometry with 'solid' material associated.

  // Methods for Reader

  inline G4bool IsValid(const G4String& name) const;
  inline G4double GetConstant(const G4String& name) const;
  inline G4double GetVariable(const G4String& name) const;
  inline G4double GetQuantity(const G4String& name) const;
  inline G4ThreeVector GetPosition(const G4String& name) const;
  inline G4ThreeVector GetRotation(const G4String& name) const;
  inline G4ThreeVector GetScale(const G4String& name) const;
  inline G4GDMLMatrix GetMatrix(const G4String& name) const;
  inline G4LogicalVolume* GetVolume(const G4String& name) const;
  inline G4VPhysicalVolume* GetPhysVolume(const G4String& name) const;
  inline G4VPhysicalVolume* GetWorldVolume(const G4String& setupName = "Default") const;
  inline G4GDMLAuxListType GetVolumeAuxiliaryInformation(G4LogicalVolume* lvol) const;
  inline const G4GDMLAuxMapType* GetAuxMap() const;
  inline const G4GDMLAuxListType* GetAuxList() const;
  inline void AddAuxiliary(G4GDMLAuxStructType myaux);
  inline void StripNamePointers() const;
  inline void SetStripFlag(G4bool);
  inline void SetOverlapCheck(G4bool);
  inline void SetRegionExport(G4bool);
  inline void SetEnergyCutsExport(G4bool);
  inline void SetSDExport(G4bool);
  inline void SetReverseSearch(G4bool);

  inline G4int GetMaxExportLevel() const;  // Manage max number of levels
  inline void SetMaxExportLevel(G4int);    // to export

  inline void Clear();  // Clears the evaluator

  // Methods for Writer

  inline void AddModule(const G4VPhysicalVolume* const physvol);
  inline void AddModule(const G4int depth);
  inline void SetAddPointerToName(G4bool set);
  inline void AddVolumeAuxiliary(G4GDMLAuxStructType myaux, const G4LogicalVolume* const lvol);
  inline void SetOutputFileOverwrite(G4bool flag);

 private:
  void ImportRegions();
  void ExportRegions(G4bool storeReferences = true);

 private:
  G4GDMLEvaluator eval;
  G4GDMLReadStructure* reader = nullptr;
  GDMLWriteStructure* writer = nullptr;
  G4GDMLAuxListType *rlist = nullptr, *ullist = nullptr;
  GDMLMessenger* messenger = nullptr;
  G4bool urcode = false, uwcode = false, strip = false, rexp = false;
};

inline void GDMLParser::Read(const G4String& filename, G4bool validate) {
  if (G4Threading::IsMasterThread()) {
    reader->Read(filename, validate, false, strip);
    ImportRegions();
  }
}

// --------------------------------------------------------------------
inline void GDMLParser::ReadModule(const G4String& filename, G4bool validate) {
  if (G4Threading::IsMasterThread()) {
    reader->Read(filename, validate, true);
    ImportRegions();
  }
}

// --------------------------------------------------------------------
inline void GDMLParser::Write(const G4String& filename, const G4VPhysicalVolume* pvol, G4bool refs,
                              const G4String& schemaLocation) {
  if (G4Threading::IsMasterThread()) {
    const G4int depth = 0;
    G4LogicalVolume* lvol = nullptr;

    if (pvol == nullptr) {
      lvol = G4TransportationManager::GetTransportationManager()
                 ->GetNavigatorForTracking()
                 ->GetWorldVolume()
                 ->GetLogicalVolume();
    } else {
      lvol = pvol->GetLogicalVolume();
    }

    if (rexp) {
      ExportRegions(refs);
    }
    writer->Write(filename, lvol, schemaLocation, depth, refs);
  }
}

// --------------------------------------------------------------------
inline void GDMLParser::Write(const G4String& filename, const G4LogicalVolume* lvol, G4bool refs,
                              const G4String& schemaLocation) {
  if (G4Threading::IsMasterThread()) {
    const G4int depth = 0;

    if (lvol == nullptr) {
      lvol = G4TransportationManager::GetTransportationManager()
                 ->GetNavigatorForTracking()
                 ->GetWorldVolume()
                 ->GetLogicalVolume();
    }
    if (rexp) {
      ExportRegions(refs);
    }
    writer->Write(filename, lvol, schemaLocation, depth, refs);
  }
}

// --------------------------------------------------------------------
inline G4LogicalVolume* GDMLParser::ParseST(const G4String& filename, G4Material* medium, G4Material* solid) {
  if (G4Threading::IsMasterThread()) {
    G4STRead STreader;
    return STreader.Read(filename, medium, solid);
  } else {
    return nullptr;
  }
}

// --------------------------------------------------------------------
// Methods for Reader
// --------------------------------------------------------------------

inline G4bool GDMLParser::IsValid(const G4String& name) const { return reader->IsValidID(name); }

inline G4double GDMLParser::GetConstant(const G4String& name) const { return reader->GetConstant(name); }

inline G4double GDMLParser::GetVariable(const G4String& name) const { return reader->GetVariable(name); }

inline G4double GDMLParser::GetQuantity(const G4String& name) const { return reader->GetQuantity(name); }

inline G4ThreeVector GDMLParser::GetPosition(const G4String& name) const { return reader->GetPosition(name); }

inline G4ThreeVector GDMLParser::GetRotation(const G4String& name) const { return reader->GetRotation(name); }

inline G4ThreeVector GDMLParser::GetScale(const G4String& name) const { return reader->GetScale(name); }

inline G4GDMLMatrix GDMLParser::GetMatrix(const G4String& name) const { return reader->GetMatrix(name); }

inline G4LogicalVolume* GDMLParser::GetVolume(const G4String& name) const { return reader->GetVolume(name); }

inline G4VPhysicalVolume* GDMLParser::GetPhysVolume(const G4String& name) const { return reader->GetPhysvol(name); }

inline G4VPhysicalVolume* GDMLParser::GetWorldVolume(const G4String& setupName) const {
  return reader->GetWorldVolume(setupName);
}

inline G4GDMLAuxListType GDMLParser::GetVolumeAuxiliaryInformation(G4LogicalVolume* logvol) const {
  return reader->GetVolumeAuxiliaryInformation(logvol);
}

inline const G4GDMLAuxMapType* GDMLParser::GetAuxMap() const { return reader->GetAuxMap(); }

inline const G4GDMLAuxListType* GDMLParser::GetAuxList() const { return reader->GetAuxList(); }

inline void GDMLParser::AddAuxiliary(G4GDMLAuxStructType myaux) { return writer->AddAuxiliary(myaux); }

inline void GDMLParser::StripNamePointers() const { reader->StripNames(); }

inline void GDMLParser::SetStripFlag(G4bool flag) { strip = flag; }

inline void GDMLParser::SetOverlapCheck(G4bool flag) { reader->OverlapCheck(flag); }

inline void GDMLParser::SetRegionExport(G4bool flag) { rexp = flag; }

inline void GDMLParser::SetEnergyCutsExport(G4bool flag) { writer->SetEnergyCutsExport(flag); }

inline void GDMLParser::SetSDExport(G4bool flag) { writer->SetSDExport(flag); }

inline void GDMLParser::SetReverseSearch(G4bool flag) { reader->SetReverseSearch(flag); }

inline G4int GDMLParser::GetMaxExportLevel() const { return writer->GetMaxExportLevel(); }

inline void GDMLParser::SetMaxExportLevel(G4int level) { writer->SetMaxExportLevel(level); }

inline void GDMLParser::Clear() { reader->Clear(); }

// --------------------------------------------------------------------
// Methods for Writer
// --------------------------------------------------------------------

inline void GDMLParser::AddModule(const G4VPhysicalVolume* const physvol) { writer->AddModule(physvol); }

inline void GDMLParser::AddModule(const G4int depth) { writer->AddModule(depth); }

inline void GDMLParser::SetAddPointerToName(G4bool set) { writer->SetAddPointerToName(set); }

inline void GDMLParser::AddVolumeAuxiliary(G4GDMLAuxStructType myaux, const G4LogicalVolume* const lvol) {
  writer->AddVolumeAuxiliary(myaux, lvol);
}

inline void GDMLParser::SetOutputFileOverwrite(G4bool flag) { writer->SetOutputFileOverwrite(flag); }

}  // namespace RAT
#endif
