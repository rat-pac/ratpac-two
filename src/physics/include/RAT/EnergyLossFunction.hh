////////////////////////////////////////////////////////////////////
/// \class EnergyLossFunction
///
/// \brief Interface to calculating material stopping power
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Interface to calculate material stopping power as a function of
///          energy. Currently, just a wrapper around G4EmCalculator, a GEANT4
///          -supported utility.
///
////////////////////////////////////////////////////////////////////

#ifndef __EnergyLossFunction__
#define __EnergyLossFunction__

#include <G4EmCalculator.hh>
#include <G4Material.hh>
#include <G4ParticleDefinition.hh>
#include <memory>

class EnergyLossFunction {
 public:
  EnergyLossFunction(){/**/};
  ~EnergyLossFunction(){/**/};
  EnergyLossFunction(const G4ParticleDefinition* _def, const G4Material* _mat);
  double Evaluate(double x);

 protected:
  /**/
 private:
  const G4ParticleDefinition* fDef;
  const G4Material* fMat;
  std::unique_ptr<G4EmCalculator> fCalculator;
};

#endif
