#ifndef __RAT_LIGHTPATHCALCULATOR_HH__
#define __RAT_LIGHTPATHCALCULATOR_HH__

#include <G4Material.hh>
#include <G4Navigator.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <G4VPhysicalVolume.hh>
#include <memory>
#include <vector>

namespace RAT {

class LightPathCalculator {
 public:
  struct Segment {
    G4ThreeVector startPos;
    G4ThreeVector endPos;
    G4double length;
    const G4Material* material;
    G4String volumeName;
  };

  virtual ~LightPathCalculator() = default;
  virtual std::vector<Segment> Navigate(const G4ThreeVector& start, const G4ThreeVector& end) const = 0;
  virtual void SetWorldVolume(G4VPhysicalVolume*) {}
};

class G4LightPathCalculator : public LightPathCalculator {
 public:
  G4LightPathCalculator() = default;

  ~G4LightPathCalculator();

  std::vector<Segment> Navigate(const G4ThreeVector& start, const G4ThreeVector& end) const override;

  void SetWorldVolume(G4VPhysicalVolume* world) override;

  // No copying (we own a navigator).
  G4LightPathCalculator(const G4LightPathCalculator&) = delete;
  G4LightPathCalculator& operator=(const G4LightPathCalculator&) = delete;

 private:
  std::unique_ptr<G4Navigator> fNavigator;
};

}  // namespace RAT

#endif
