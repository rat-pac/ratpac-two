#ifndef __RAT_PMTInfoParser__
#define __RAT_PMTInfoParser__

#include <G4RotationMatrix.hh>
#include <RAT/DB.hh>
#include <RAT/PMTConstruction.hh>
#include <vector>

namespace RAT {

// Helper class which reads the attributes stored in a PMTINFO
class PMTInfoParser {
 public:
  PMTInfoParser(DBLinkPtr table, const std::string &mother_name);
  ~PMTInfoParser(){};

  // Returns total number of PMTs described by the PMTINFO table
  int GetPMTCount() const { return fPos.size(); };

  // Returns a vector of PMT positions in the global coordinate system
  const std::vector<G4ThreeVector> &GetPMTLocations() const { return fPos; };

  // Returns the offset between local and global coordinates
  // e.g. local = global - offset
  G4ThreeVector GetLocalOffset() { return fLocalOffset; }
  static G4ThreeVector ComputeLocalOffset(const std::string &);

  // Returns the direction vector of the front face of the PMTs
  const std::vector<G4ThreeVector> &GetPMTDirections() const { return fDir; };

  // Returns the rotation matrix required to orient PMT of index i
  // in a G4PVPlacement.  By GEANT4 convention, this is a passive rotation.
  // Use the invert() method to convert it to an active rotation if you
  // want to apply it to a vector.
  G4RotationMatrix GetPMTRotation(int i) const;

  const std::vector<double> &GetEfficiencyCorrections() { return fEfficiencyCorrection; }
  const std::vector<double> &GetPMTNoiseRates() { return fNoiseRate; }
  const std::vector<double> &GetPMTAfterPulseFraction() { return fAfterPulseFraction; }
  const std::vector<int> &GetTypes() { return fType; }
  const std::vector<int> &GetChannelNumbers() { return fChannelNumber; }

 protected:
  G4ThreeVector fLocalOffset;
  std::vector<G4ThreeVector> fPos;
  std::vector<G4ThreeVector> fDir;
  std::vector<int> fChannelNumber;
  std::vector<int> fType;
  std::vector<double> fEfficiencyCorrection;
  std::vector<double> fNoiseRate;
  std::vector<double> fAfterPulseFraction;
};
}  // namespace RAT
#endif
