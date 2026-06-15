#ifndef __RAT_TRANSITTIMECALCULATOR_HH__
#define __RAT_TRANSITTIMECALCULATOR_HH__

#include <TVector3.h>

#include <G4VPhysicalVolume.hh>
#include <RAT/GroupVelocityCalculator.hh>
#include <RAT/LightPathCalculator.hh>
#include <string>
#include <vector>

namespace RAT {

class TransitTimeCalculator {
 public:
  struct Segment {
    double velocity;
    std::string materialName;
    TVector3 startPos;
    TVector3 endPos;
  };

  struct Result {
    double totalTime = 0.;
    double totalLength = 0.;
    std::vector<Segment> segments;
  };

  explicit TransitTimeCalculator(const std::string& pathCalcId = "g4", const std::string& velocityCalcId = "rindex",
                                 G4VPhysicalVolume* worldVolume = nullptr, double defaultRindex = 1.0);

  ~TransitTimeCalculator();

  // No copying or moving (owns opaque sub-calculators).
  TransitTimeCalculator(const TransitTimeCalculator&) = delete;
  TransitTimeCalculator& operator=(const TransitTimeCalculator&) = delete;
  TransitTimeCalculator(TransitTimeCalculator&&) = delete;
  TransitTimeCalculator& operator=(TransitTimeCalculator&&) = delete;

  // Walk the straight line from start to end and return the transit time breakdown.
  // wavelength_nm is the photon wavelength in nanometres.
  // If full_output is true the per-segment breakdown is populated with material
  // name, start/end positions, length, and velocity.
  Result Compute(const TVector3& start, const TVector3& end, double wavelength_nm = 400,
                 bool full_output = false) const;

  void SetVerbose(bool v) { fVerbose = v; }
  void SetDefaultRindex(double n);

 private:
  std::unique_ptr<LightPathCalculator> fPathCalc;
  std::unique_ptr<GroupVelocityCalculator> fVelocityCalc;
  bool fVerbose = false;
};

}  // namespace RAT

#endif
