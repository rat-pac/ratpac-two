#include "RAT/LightPathCalculator.hh"

#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4Navigator.hh"
#include "G4SystemOfUnits.hh"
#include "G4TransportationManager.hh"
#include "G4UnitsTable.hh"
#include "G4VPhysicalVolume.hh"
#include "RAT/Log.hh"

namespace RAT {

G4LightPathCalculator::~G4LightPathCalculator() = default;

void G4LightPathCalculator::SetWorldVolume(G4VPhysicalVolume* world) {
  G4VPhysicalVolume* w = world;
  if (w == nullptr) {
    w = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->GetWorldVolume();
  }
  if (w == nullptr) {
    Log::Die(
        "G4LightPathCalculator: no world volume provided and none found in the transportation manager. "
        "No G4 geometry is available for navigation.");
  }
  fNavigator = std::make_unique<G4Navigator>();
  fNavigator->SetWorldVolume(w);
}

std::vector<LightPathCalculator::Segment> G4LightPathCalculator::Navigate(const G4ThreeVector& start,
                                                                          const G4ThreeVector& end) const {
  std::vector<Segment> segments;

  if (fNavigator == nullptr) {
    Log::Die("G4LightPathCalculator::Navigate: navigator not initialised. Call SetWorldVolume first.");
  }

  const G4ThreeVector delta = end - start;
  const G4double totalDistance = delta.mag();
  if (totalDistance <= 0.) {
    return segments;
  }
  const G4ThreeVector direction = delta.unit();

  const G4double kPush = 1.0e-7 * mm;

  G4ThreeVector point = start;
  G4double traveled = 0.;

  G4VPhysicalVolume* volume =
      fNavigator->LocateGlobalPointAndSetup(point, &direction, /*pRelativeSearch=*/false, /*ignoreDirection=*/false);

  if (volume == nullptr) {
    warn << "[G4LightPathCalculator] WARNING: start point "
            "("
         << point.x() / mm << " mm, " << point.y() / mm << " mm, " << point.z() / mm << " mm) "
         << " is outside the world volume." << newline;
  }

  const G4int kMaxSteps = 10000000;
  G4int nSteps = 0;
  G4int tinySteps = 0;

  G4ThreeVector segStart = point;

  while (traveled < totalDistance && nSteps < kMaxSteps) {
    ++nSteps;

    const G4double remaining = totalDistance - traveled;
    G4double safety = 0.;

    G4double step = fNavigator->ComputeStep(point, direction, remaining, safety);

    G4bool limitedByGeometry = true;
    if (step == kInfinity) {
      step = remaining;
      limitedByGeometry = false;
    }
    if (step >= remaining) {
      step = remaining;
      limitedByGeometry = false;
    }

    G4String volName = "OUTSIDE_WORLD";
    const G4Material* material = nullptr;

    if (volume != nullptr) {
      volName = volume->GetName();
      material = volume->GetLogicalVolume()->GetMaterial();
    }

    Segment seg;
    seg.startPos = segStart;
    point += step * direction;
    seg.endPos = point;
    seg.length = step;
    seg.material = material;
    seg.volumeName = volName;
    segments.push_back(seg);

    traveled += step;
    segStart = point;

    if (!limitedByGeometry) break;

    fNavigator->SetGeometricallyLimitedStep();
    volume = fNavigator->LocateGlobalPointAndSetup(point, &direction, /*pRelativeSearch=*/true,
                                                   /*ignoreDirection=*/false);

    if (step < kPush) {
      if (++tinySteps > 3 && traveled < totalDistance) {
        point += kPush * direction;
        traveled += kPush;
        segStart = point;
        volume = fNavigator->LocateGlobalPointAndSetup(point, &direction, false, false);
        tinySteps = 0;
      }
    } else {
      tinySteps = 0;
    }
  }

  if (nSteps >= kMaxSteps) {
    G4cerr << "[G4LightPathCalculator] WARNING: step limit reached; result may be incomplete." << G4endl;
  }

  return segments;
}

}  // namespace RAT
