#include <RAT/AppliedQuenchingModel.hh>
#include <memory>

AppliedQuenchingModel::AppliedQuenchingModel() : kB(0.0) { /* */ }

AppliedQuenchingModel::AppliedQuenchingModel(const BirksLaw _model, const G4ParticleDefinition* def,
                                             const G4Material* mat, const double _kB)
    : model(_model), kB(_kB) {
  this->loss = std::unique_ptr<EnergyLossFunction>(new EnergyLossFunction(def, mat));
}

double AppliedQuenchingModel::Evaluate(const double x) {
  const double dEdx = this->loss->Evaluate(x);
  const double rv = this->model.Evaluate(x, dEdx, this->kB);
  return rv;
}
