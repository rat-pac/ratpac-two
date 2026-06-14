#include <TFile.h>
#include <TObjString.h>

#include <G4RunManager.hh>
#include <G4StateManager.hh>
#include <RAT/DB.hh>
#include <RAT/DBJsonLoader.hh>
#include <RAT/DBTable.hh>
#include <RAT/DS/RootFactory.hh>
#include <RAT/DSReader.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/json.hh>
#include <iostream>

namespace RAT {

#undef DEBUG

DB *DSReader::LoadDB() {
  DB *db = DB::Get();
  db->Set("DB", "is_snapshot", true);

  // The outroot processor stores the resolved DB as a JSON-array TObjString
  // named "ratdb" in each output file (see OutROOTProc). Pull it from the
  // first file of the chain.
  if (total > 0) T.LoadTree(0);
  TFile *file = T.GetCurrentFile();
  if (file == nullptr) {
    warn << "DSReader::LoadDB: no input file available; DB left untouched." << newline;
    return db;
  }

  TObjString *snapshot = dynamic_cast<TObjString *>(file->Get("ratdb"));
  if (snapshot == nullptr) {
    warn << "DSReader::LoadDB: '" << file->GetName()
         << "' has no embedded RATDB snapshot ('ratdb'); was it written by an older RAT?" << newline;
    return db;
  }

  // DumpContentsToJson writes a JSON array of table objects, so unpack the
  // array and feed each element through the normal RATDB load path.
  json::Reader reader(snapshot->GetString().Data());
  json::Value doc;
  int nTables = 0;
  if (reader.getValue(doc) && doc.getType() == json::TARRAY) {
    for (size_t i = 0; i < doc.getArraySize(); i++) {
      json::Value entry = doc.getIndex(i);
      db->LoadTable(DBJsonLoader::convertTable(entry));
      ++nTables;
    }
  } else {
    warn << "DSReader::LoadDB: unexpected RATDB snapshot format in '" << file->GetName() << "'." << newline;
  }

  info << "DSReader::LoadDB: loaded " << nTables << " RATDB tables from " << file->GetName() << newline;
  return db;
}

void DSReader::BuildGeometry() {
  // The geometry tables come from the DB; LoadDB() runs in the constructor.
  G4RunManager *runManager = G4RunManager::GetRunManager();
  if (runManager != nullptr) {
    warn << "DSReader::BuildGeometry: a G4RunManager already exists; geometry left untouched." << newline;
    return;
  }

  // Mirror rat's /run/initialize: a run manager needs both a physics list and
  // a detector construction before Initialize() builds and closes the geometry.
  runManager = new G4RunManager;
  runManager->SetUserInitialization(new PhysicsList());
  runManager->SetUserInitialization(DetectorConstruction::GetDetectorConstruction());
  runManager->Initialize();

  info << "DSReader::BuildGeometry: detector geometry constructed and closed." << newline;
}

DSReader::DSReader(const char *filename) : T("T"), runT("runT") {
  RAT::Log::Init("/dev/null");
  T.Add(filename);

  next = 0;
  total = T.GetEntries();

#ifdef DEBUG
  debug << "DSReader::DSReader - "
        << "filename='" << filename << "', total=" << total << newline;
#endif

  ds = new DS::Root();
  T.SetBranchAddress("ds", &ds);

  // Reconstruct the DB from this file's snapshot before any event looping.
  LoadDB();
}

DSReader::~DSReader() { delete ds; }

void DSReader::Add(const char *filename) {
  T.Add(filename);
  runT.Add(filename);
  total = T.GetEntries();

#ifdef DEBUG
  debug << "DSReader::Add - "
        << "filename='" << filename << "', total=" << total << newline;
#endif
}

DS::Root *DSReader::NextEvent() {
  if (next < total) {
    T.GetEntry(next);
    next++;
    return ds;
  } else
    return 0;
}

}  // namespace RAT
