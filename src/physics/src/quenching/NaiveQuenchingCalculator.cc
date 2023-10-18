#include <RAT/NaiveQuenchingCalculator.hh>

NaiveQuenchingCalculator::NaiveQuenchingCalculator(BirksLaw model) : QuenchingCalculator(model) { /* */ }

double NaiveQuenchingCalculator::QuenchedEnergyDeposit(const G4Step& step, const double kB) {
  const double E = step.GetPreStepPoint()->GetKineticEnergy();
  const double dE = step.GetTotalEnergyDeposit();
  const double dX = step.GetStepLength();
  const double dEdx = dE / dX;
  const double quenching = this->model.Evaluate(E, dEdx, kB);
  const double rv = dE * quenching;
  return rv;
}
