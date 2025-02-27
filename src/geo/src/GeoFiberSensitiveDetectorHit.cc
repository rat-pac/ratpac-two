#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Circle.hh>
#include <G4Colour.hh>
#include <G4VVisManager.hh>
#include <G4VisAttributes.hh>
#include <G4ios.hh>
#include <RAT/GeoFiberSensitiveDetectorHit.hh>
#include <RAT/Log.hh>

namespace RAT {

G4Allocator<GeoFiberSensitiveDetectorHit> GeoFiberSensitiveDetectorHitAllocator;

GeoFiberSensitiveDetectorHit::GeoFiberSensitiveDetectorHit(G4int i, G4double t, G4ThreeVector p) {
  id = i;
  time = t;
  hit_pos = p;
  pLogV = 0;
}
GeoFiberSensitiveDetectorHit::~GeoFiberSensitiveDetectorHit() { ; }

GeoFiberSensitiveDetectorHit::GeoFiberSensitiveDetectorHit(const GeoFiberSensitiveDetectorHit &right) : G4VHit() {
  id = right.id;
  time = right.time;
  pos = right.pos;
  hit_pos = right.hit_pos;
  rot = right.rot;
  pLogV = right.pLogV;
}

const GeoFiberSensitiveDetectorHit &GeoFiberSensitiveDetectorHit::operator=(const GeoFiberSensitiveDetectorHit &right) {
  id = right.id;
  time = right.time;
  pos = right.pos;
  hit_pos = right.hit_pos;
  rot = right.rot;
  pLogV = right.pLogV;
  return *this;
}

int GeoFiberSensitiveDetectorHit::operator==(const GeoFiberSensitiveDetectorHit & /*right*/) const { return 0; }

void GeoFiberSensitiveDetectorHit::Draw() {
  G4VVisManager *pVVisManager = G4VVisManager::GetConcreteInstance();
  if (pVVisManager) {
    G4Transform3D trans(rot.inverse(), pos);
    G4VisAttributes attribs;
    const G4VisAttributes *pVA = pLogV->GetVisAttributes();
    if (pVA) attribs = *pVA;
    G4Colour colour(0., 1., 1.);
    attribs.SetColour(colour);
    attribs.SetForceSolid(true);
    pVVisManager->Draw(*pLogV, attribs, trans);
  }
}

void GeoFiberSensitiveDetectorHit::Print() {
  debug << "  GeoFiberSensitiveDetector[" << id << "] " << time / CLHEP::ns << " (nsec)" << newline;
}

}  // namespace RAT
