#include "RAT/TransitTimeCalculator.hh"

#include "G4Material.hh"
#include "G4PhysicalConstants.hh"
#include "G4ThreeVector.hh"
#include "G4UnitsTable.hh"
#include "RAT/Factory.hh"
#include "RAT/GroupVelocityCalculator.hh"
#include "RAT/LightPathCalculator.hh"
#include "RAT/Log.hh"
#include "TVector3.h"

namespace RAT {

TransitTimeCalculator::TransitTimeCalculator(const std::string& pathCalcId, const std::string& velocityCalcId,
                                             G4VPhysicalVolume* worldVolume, double defaultRindex) {
  static bool registered = false;
  if (!registered) {
    GlobalFactory<LightPathCalculator>::Register("g4", new Alloc<LightPathCalculator, G4LightPathCalculator>);
    GlobalFactory<GroupVelocityCalculator>::Register("rindex",
                                                     new Alloc<GroupVelocityCalculator, RindexGroupVelocityCalculator>);
    registered = true;
  }

  fPathCalc.reset(GlobalFactory<LightPathCalculator>::New(pathCalcId));
  fVelocityCalc.reset(GlobalFactory<GroupVelocityCalculator>::New(velocityCalcId));
  fPathCalc->SetWorldVolume(worldVolume);
  fVelocityCalc->SetDefaultRindex(defaultRindex);
}

TransitTimeCalculator::~TransitTimeCalculator() = default;

void TransitTimeCalculator::SetDefaultRindex(double n) { fVelocityCalc->SetDefaultRindex(n); }

TransitTimeCalculator::Result TransitTimeCalculator::Compute(const TVector3& start, const TVector3& end,
                                                             double wavelength_nm, bool full_output) const {
  Result result;

  const G4ThreeVector g4Start(start.X(), start.Y(), start.Z());
  const G4ThreeVector g4End(end.X(), end.Y(), end.Z());

  const std::vector<LightPathCalculator::Segment> pathSegments = fPathCalc->Navigate(g4Start, g4End);

  for (const auto& ps : pathSegments) {
    const GroupVelocityCalculator::Result vel_res =
        fVelocityCalc->ComputeVelocity(ps.material, wavelength_nm, ps.startPos, ps.endPos);

    double segTime = 0.;
    double velocity = 0.;
    switch (vel_res.status) {
      case GroupVelocityCalculator::Status_t::kNormal:
        velocity = vel_res.velocity;
        Log::Assert(velocity > 0., "[TransitTimeCalculator] ERROR: computed velocity is non-positive.");
        segTime = ps.length / velocity;
        result.totalLength += ps.length;
        result.totalTime += segTime;
        break;
      case GroupVelocityCalculator::Status_t::kSkip:
        debug << "[TransitTimeCalculator] DEBUG: skipping segment in volume " << ps.volumeName
              << " due to velocity calculator request." << newline;
        break;
      case GroupVelocityCalculator::Status_t::kStop:
        debug << "[TransitTimeCalculator] DEBUG: stopping track in volume " << ps.volumeName
              << " due to velocity calculator request." << newline;
        return result;
    }

    if (full_output) {
      Segment seg;
      seg.velocity = velocity;
      seg.materialName = ps.material ? std::string(ps.material->GetName()) : std::string("undef");
      seg.startPos = TVector3(ps.startPos.x(), ps.startPos.y(), ps.startPos.z());
      seg.endPos = TVector3(ps.endPos.x(), ps.endPos.y(), ps.endPos.z());
      result.segments.push_back(seg);
    }

    if (fVerbose) {
      const G4String& matName = ps.material ? ps.material->GetName() : G4String("undef");
      const double n = (velocity > 0.) ? c_light / velocity : 0.;
      G4cout << "[TransitTimeCalculator]  vol=" << ps.volumeName << "  mat=" << matName
             << "  L=" << G4BestUnit(ps.length, "Length") << "  n=" << n << "  t=" << G4BestUnit(segTime, "Time")
             << G4endl;
    }
  }

  return result;
}

}  // namespace RAT
