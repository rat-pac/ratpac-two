#include <RAT/EnergyLossFunction.hh>
#include <memory>

EnergyLossFunction::EnergyLossFunction(const G4ParticleDefinition* _def, const G4Material* _mat)
    : fDef(_def), fMat(_mat) {
  this->fCalculator = std::unique_ptr<G4EmCalculator>(new G4EmCalculator());
}

double EnergyLossFunction::Evaluate(double x) {
  double rv = this->fCalculator->GetDEDX(x, this->fDef, this->fMat);
  return rv;
}
