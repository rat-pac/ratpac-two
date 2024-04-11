

///////////////////////////////////////////////////////////////////////////////
//                   Spectrum of radiative neutron capture by Gadolinium
//                                    version 1.0.0
//                                    (Sep.09.2005)

// Modified class from original G4NeutronHPCapture class to include
// the gammas spectrum of radiative neutron capture by Gadolinium.

// Karim Zbiri, April, 2005
///////////////////////////////////////////////////////////////////////////////

#ifndef GdNeutronHPCapture_h
#define GdNeutronHPCapture_h 1

#include "G4HadronicInteraction.hh"
#include "G4ParticleHPChannel.hh"
#include "globals.hh"

class GdNeutronHPCapture : public G4HadronicInteraction {
 public:
  GdNeutronHPCapture();
  ~GdNeutronHPCapture() override;

  G4HadFinalState* ApplyYourself(const G4HadProjectile& aTrack, G4Nucleus& aTargetNucleus) override;

  const std::pair<G4double, G4double> GetFatalEnergyCheckLevels() const override;
  G4int GetVerboseLevel() const;
  void SetVerboseLevel(G4int);
  void BuildPhysicsTable(const G4ParticleDefinition&) override;
  void ModelDescription(std::ostream& outFile) const override;

 private:
  std::vector<G4ParticleHPChannel*>* theCapture{nullptr};
  G4String dirName;
  G4int numEle{0};
  G4HadFinalState theResult;
};

#endif
