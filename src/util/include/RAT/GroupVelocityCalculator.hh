#ifndef __RAT_GROUPVELOCITYCALCULATOR_HH__
#define __RAT_GROUPVELOCITYCALCULATOR_HH__

#include <G4Material.hh>
#include <G4ThreeVector.hh>
namespace RAT {

class GroupVelocityCalculator {
 public:
  enum class Status_t { kNormal, kSkip, kStop };
  struct Result {
    double velocity;
    Status_t status;
  };
  virtual ~GroupVelocityCalculator() = default;
  virtual Result ComputeVelocity(const G4Material* material, G4double wavelength_nm, const G4ThreeVector& startPos,
                                 const G4ThreeVector& endPos) const = 0;
  virtual void SetDefaultRindex(G4double) {}
};

class RindexGroupVelocityCalculator : public GroupVelocityCalculator {
 public:
  RindexGroupVelocityCalculator() = default;

  Result ComputeVelocity(const G4Material* material, G4double wavelength_nm, const G4ThreeVector& startPos,
                         const G4ThreeVector& endPos) const override;

  void SetDefaultRindex(G4double n) override { fDefaultRindex = n; }

 private:
  G4double fDefaultRindex = 1.0;
};

}  // namespace RAT

#endif
