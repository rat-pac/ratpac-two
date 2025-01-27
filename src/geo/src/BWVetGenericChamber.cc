//
// $Id: BWVetGenericChamber.cc,v 1.2 2005/11/01 04:44:21 sekula Exp $
// --------------------------------------------------------------
//
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
#include <RAT/BWVetGenericChamber.hh>
#include <RAT/BWVetGenericChamberHit.hh>
#include <RAT/Log.hh>

namespace RAT {

BWVetGenericChamber::BWVetGenericChamber(G4String name) : G4VSensitiveDetector(name) {
  G4String HCname;
  collectionName.insert(HCname = "genericchamberColl");
  HCID = -1;
}

BWVetGenericChamber::~BWVetGenericChamber() { ; }

void BWVetGenericChamber::Initialize(G4HCofThisEvent *HCE) {
  int deb = 0;  // G4int(db["veto_debugging"]);
  if (deb) RAT::debug << "BWVetGenericChamber::Initialize start." << newline;
  _hitsCollection = new BWVetGenericChamberHitsCollection(SensitiveDetectorName, collectionName[0]);

  if (deb) {
    RAT::debug << "BWVetGenericChamber::Initialize hit collection address is " << _hitsCollection << newline;
  }
  if (HCID < 0) {
    HCID = G4SDManager::GetSDMpointer()->GetCollectionID(_hitsCollection);
  }
  if (deb) RAT::debug << "BWVetGenericChamber::Initialize hit collection ID = " << HCID << newline;
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

  if (deb) RAT::debug << "BWVetGenericChamber::Initialize end." << newline;
}

G4bool BWVetGenericChamber::ProcessHits(G4Step *aStep, G4TouchableHistory * /*ROhist*/) {
  // Don't store optical photons!
  if (aStep->GetTrack()->GetDefinition()->GetParticleName() == "opticalphoton")
    return true;  // method description neglects to say what retval is for

  int deb = 0;  // G4int(db["veto_debugging"]);

  if (deb) {
    RAT::debug << "BWVetGenericChamber::ProcessHits start." << newline;
    RAT::debug << "BWVetGenericChamber::ProcessHits getting energy deposited." << newline;
  }
  G4double edep = aStep->GetTotalEnergyDeposit();
  G4double dl = aStep->GetStepLength();

  if (deb) RAT::debug << "   Energy deposited: " << edep << newline;

  //   if(edep==0.) return true;

  G4Track *aTrack = aStep->GetTrack();
  G4ParticleDefinition *part = aTrack->GetDefinition();
  int pdg = part->GetPDGEncoding();

  if (deb)
    RAT::debug << "  track information: " << newline << "    G4Track Pointer: " << aTrack << newline
               << "    Particle Definition Pointer: " << part << newline << "    Particle PDG Encoding: " << pdg
               << newline;

  if (deb) RAT::debug << "BWVetGenericChamber::ProcessHits getting global time." << newline;
  G4StepPoint *preStepPoint = aStep->GetPreStepPoint();
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

  if (deb) RAT::debug << "History level: " << theTouchable->GetHistoryDepth() << newline;

  while (ivol < theTouchable->GetHistoryDepth()) {
    if (deb) RAT::debug << " * volume layer level = " << ivol << newline;
    uid += theTouchable->GetVolume(ivol)->GetCopyNo() * idOffset;
    idOffset *= 100;
    ivol++;
  }

  G4ThreeVector worldPos = preStepPoint->GetPosition();

  if (deb) {
    RAT::debug << "Hit material name " << mname << newline;
    RAT::debug << "density           " << d << newline;
    RAT::debug << "formula           " << f << newline;
    RAT::debug << "edep " << G4BestUnit(edep, "Energy") << newline;
    RAT::debug << "dl " << G4BestUnit(dl, "Length") << newline;
    RAT::debug << "pid " << pdg << newline;
    RAT::debug << "Position " << G4BestUnit(worldPos.x(), "Length") << " " << G4BestUnit(worldPos.y(), "Length") << " "
               << G4BestUnit(worldPos.z(), "Length") << " " << newline;

    RAT::debug << " " << newline;
  }

  G4double hitTime = preStepPoint->GetGlobalTime();

  if (deb)
    RAT::debug << "BWVetGenericChamber::ProcessHits checking for an existing hit "
                  "in this element."
               << newline;
  // check if this finger already has a hit
  G4int ix = -1;

  if (deb) {
    RAT::debug << "BWVetGenericChamber::ProcessHits hit collection address is " << _hitsCollection << newline;
    G4cerr << "BWVetGenericChamber: Hit ID = " << uid << " and position: " << worldPos << newline;
  }

  int eventID = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEvent();
  int trackID = aStep->GetTrack()->GetTrackID();

  // get volume name of hit as well
  // note that this volume name may not be unique
  G4String vol = theTouchable->GetVolume()->GetName();

  if (fLastEventID != eventID || fLastTrackID != trackID) {
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
    if (deb)
      RAT::debug << "BWVetGenericChamber::ProcessHits hit collection null. "
                    "Reloading from HCofEThisEvent."
                 << newline;
    if (_HCE) {
      _hitsCollection = (BWVetGenericChamberHitsCollection *)(_HCE->GetHC(HCID));
      if (deb) {
        RAT::debug << "BWVetGenericChamber::ProcessHits   * hit collection address is " << _hitsCollection << newline;
      }
    } else {
      if (deb)
        RAT::debug << "BWVetGenericChamber::ProcessHits   (E) HCofEThisEvent "
                      "pointer is NULL!"
                   << newline;
    }
  }

  if (_hitsCollection) {
    for (size_t i = 0; i < _hitsCollection->entries(); i++) {
      // 	RAT::debug << "  * BWVetGenericChamber::ProcessHits checking hit "
      // 	       << i + 1
      // 	       << " of "
      // 	       << _hitsCollection->entries()
      // 	       << newline;
      if (deb) RAT::debug << "  * this hit ID is " << (*_hitsCollection)[i]->GetID() << newline;
      if ((*_hitsCollection)[i]->GetID() == uid) {
        ix = i;
        break;
      }
    }

    // if it has, then take the earlier time
    if (ix >= 0) {
      if (deb)
        RAT::debug << "BWVetGenericChamber::ProcessHits use existing earlier time "
                      "for hit."
                   << newline;
      if ((*_hitsCollection)[ix]->GetTime() > hitTime) {
        (*_hitsCollection)[ix]->SetTime(hitTime);
      }
    } else
    // if not, create a new hit and std::set it to the collection
    {
      if (deb) RAT::debug << "BWVetGenericChamber::ProcessHits creating a new hit." << newline;
      BWVetGenericChamberHit *aHit = new BWVetGenericChamberHit(uid, hitTime);
      G4VPhysicalVolume *thePhysical = theTouchable->GetVolume();
      aHit->SetLogV(thePhysical->GetLogicalVolume());
      G4AffineTransform aTrans = theTouchable->GetHistory()->GetTopTransform();
      aTrans.Invert();
      aHit->SetRot(aTrans.NetRotation());
      aHit->SetPos(aTrans.NetTranslation());
      _hitsCollection->insert(aHit);
      aHit->Print();
      aHit->Draw();
      if (deb) RAT::debug << "  * Drawing Hit " << uid << newline;
    }
  }
  if (deb) RAT::debug << "BWVetGenericChamber::ProcessHits end." << newline;
  return true;
}

void BWVetGenericChamber::EndOfEvent(G4HCofThisEvent * /*HCE*/) { ; }

}  // namespace RAT
