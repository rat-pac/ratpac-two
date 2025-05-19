#include <G4HCofThisEvent.hh>
#include <G4Run.hh>
#include <G4RunManager.hh>
#include <G4SDManager.hh>
#include <G4Step.hh>
#include <G4TouchableHistory.hh>
#include <G4Track.hh>
#include <G4UIcommand.hh>
#include <G4UImanager.hh>
#include <G4UnitsTable.hh>
#include <G4ios.hh>
#include <RAT/GeoFiberSensitiveDetector.hh>
#include <RAT/GeoFiberSensitiveDetectorHit.hh>
#include <RAT/Log.hh>

namespace RAT {

GeoFiberSensitiveDetector::GeoFiberSensitiveDetector(G4String name) : G4VSensitiveDetector(name) {
  G4String HCname;
  collectionName.insert(HCname = "FiberSenDet");
  HCID = -1;
}

GeoFiberSensitiveDetector::~GeoFiberSensitiveDetector() { ; }

void GeoFiberSensitiveDetector::Initialize(G4HCofThisEvent *HCE) {
  debug << "GeoFiberSensitiveDetector::Initialize start." << newline;
  _hitsCollection = new GeoFiberSensitiveDetectorHitsCollection(SensitiveDetectorName, collectionName[0]);

  debug << "GeoFiberSensitiveDetector::Initialize hit collection address is " << _hitsCollection << newline;
  if (HCID < 0) {
    HCID = G4SDManager::GetSDMpointer()->GetCollectionID(_hitsCollection);
  }
  debug << "GeoFiberSensitiveDetector::Initialize hit collection ID = " << HCID << newline;
  HCE->AddHitsCollection(HCID, _hitsCollection);

  // store pointer to hit collection
  _HCE = HCE;

  //
  // Initialize the data members used to store information for the RAT Event
  //

  // Empty the hit information std::vectors
  _hit_x.clear();
  _hit_y.clear();
  _hit_z.clear();
  _hit_E.clear();
  _hit_time.clear();
  _hit_uid.clear();
  _hit_pdg.clear();
  _hit_volume.clear();
  fLastTrackID = fLastEventID = -1;

  debug << "GeoFiberSensitiveDetector::Initialize end." << newline;
}

G4bool GeoFiberSensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory * /*ROhist*/) {
  // We ONLY want to store optical photons
  if (aStep->GetTrack()->GetDefinition()->GetParticleName() == "opticalphoton") {
    // only store information from steps where the photon is attenuated
    G4StepPoint *preStepPoint = aStep->GetPreStepPoint();
    G4StepPoint *postStepPoint = aStep->GetPostStepPoint();
    std::string proc = postStepPoint->GetProcessDefinedStep()->GetProcessName();
    if (proc != "Attenuation") return true;

    debug << "GeoFiberSensitiveDetector::ProcessHits start." << newline;
    debug << "GeoFiberSensitiveDetector::ProcessHits getting energy deposited." << newline;

    G4double edep = aStep->GetTotalEnergyDeposit();
    G4double dl = aStep->GetStepLength();

    debug << "   Energy deposited: " << edep << newline;

    //   if(edep==0.) return true;

    G4Track *aTrack = aStep->GetTrack();
    G4ParticleDefinition *part = aTrack->GetDefinition();
    int pdg = part->GetPDGEncoding();

    debug << "  track information: " << newline << "    G4Track Pointer: " << aTrack << newline
          << "    Particle Definition Pointer: " << part << newline << "    Particle PDG Encoding: " << pdg << newline;

    debug << "GeoFiberSensitiveDetector::ProcessHits getting global time." << newline;

    G4Material *m = preStepPoint->GetMaterial();
    G4String mname = m->GetName();
    G4double d = m->GetDensity();
    G4String f = m->GetChemicalFormula();

    G4TouchableHistory *theTouchable = (G4TouchableHistory *)(preStepPoint->GetTouchable());
    // Get the copy number of this element. This may not be unique,
    // if the mother is also a replica and occurs more than once.
    G4int copyNo = theTouchable->GetVolume()->GetCopyNo();
    // Get the mother copy number, and offset it by 1000. This will help
    // form the basis of the chamber ID.
    G4int motherCopyNo = (theTouchable->GetVolume(1)->GetCopyNo()) + 1000;
    G4int uid = motherCopyNo + copyNo;  // unique identifier of chamber

    // construct a unique identifier for this copy
    G4int ivol = 0;
    G4int idOffset = 1;
    uid = 0;

    debug << "History level: " << theTouchable->GetHistoryDepth() << newline;

    while (ivol < theTouchable->GetHistoryDepth()) {
      debug << " * volume layer level = " << ivol << newline;
      uid += theTouchable->GetVolume(ivol)->GetCopyNo() * idOffset;
      idOffset *= 100;
      ivol++;
    }

    G4ThreeVector worldPos = preStepPoint->GetPosition();

    debug << "Hit material name " << mname << newline;
    debug << "density           " << d << newline;
    debug << "formula           " << f << newline;
    debug << "edep " << G4BestUnit(edep, "Energy") << newline;
    debug << "dl " << G4BestUnit(dl, "Length") << newline;
    debug << "pid " << pdg << newline;
    debug << "Position " << G4BestUnit(worldPos.x(), "Length") << " " << G4BestUnit(worldPos.y(), "Length") << " "
          << G4BestUnit(worldPos.z(), "Length") << " " << newline;

    debug << " " << newline;

    G4double hitTime = preStepPoint->GetGlobalTime();

    debug << "GeoFiberSensitiveDetector::ProcessHits checking for an existing hit "
             "in this element."
          << newline;
    // check if this finger already has a hit

    debug << "GeoFiberSensitiveDetector::ProcessHits hit collection address is " << _hitsCollection << newline;
    // G4cerr << "GeoFiberSensitiveDetector: Hit ID = " << uid << " and position: " << worldPos << newline;

    int eventID = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEvent();
    int trackID = aStep->GetTrack()->GetTrackID();

    // get volume name of hit as well
    // note that this volume name may not be unique
    G4String vol = theTouchable->GetVolume()->GetName();

    // if (fLastEventID != eventID || fLastTrackID != trackID) {
    if (fLastTrackID != trackID) {
      // Fill the hit information
      _hit_x.push_back(worldPos.x());
      _hit_y.push_back(worldPos.y());
      _hit_z.push_back(worldPos.z());
      _hit_E.push_back(edep);
      _hit_time.push_back(hitTime);
      _hit_uid.push_back(uid);
      _hit_pdg.push_back(pdg);
      _hit_volume.push_back(vol);
      fLastEventID = eventID;
      fLastTrackID = trackID;
    }

    /*G4int hitfreq = G4int(db["veto_hit_frequency"]);*/

    if (NULL == _hitsCollection) {
      debug << "GeoFiberSensitiveDetector::ProcessHits hit collection null. "
               "Reloading from HCofEThisEvent."
            << newline;
      if (_HCE) {
        _hitsCollection = (GeoFiberSensitiveDetectorHitsCollection *)(_HCE->GetHC(HCID));
        debug << "GeoFiberSensitiveDetector::ProcessHits   * hit collection address is " << _hitsCollection << newline;
      } else {
        debug << "GeoFiberSensitiveDetector::ProcessHits   (E) HCofEThisEvent "
                 "pointer is NULL!"
              << newline;
      }
    }

    if (_hitsCollection) {
      debug << "GeoFiberSensitiveDetector::ProcessHits creating a new hit." << newline;
      G4ThreeVector pos = {worldPos.x(), worldPos.y(), worldPos.z()};
      GeoFiberSensitiveDetectorHit *aHit = new GeoFiberSensitiveDetectorHit(uid, hitTime, pos);
      G4VPhysicalVolume *thePhysical = theTouchable->GetVolume();
      aHit->SetLogV(thePhysical->GetLogicalVolume());
      G4AffineTransform aTrans = theTouchable->GetHistory()->GetTopTransform();
      aTrans.Invert();
      aHit->SetRot(aTrans.NetRotation());
      aHit->SetPos(aTrans.NetTranslation());
      _hitsCollection->insert(aHit);
      // aHit->Print();
      // aHit->Draw();
      debug << "  * Drawing Hit " << uid << newline;
    }
    debug << "GeoFiberSensitiveDetector::ProcessHits end." << newline;
    return true;
  }
  return true;
}

void GeoFiberSensitiveDetector::EndOfEvent(G4HCofThisEvent * /*HCE*/) { ; }

}  // namespace RAT
