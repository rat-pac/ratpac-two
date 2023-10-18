#include <RAT/BirksLaw.hh>

double BirksLaw::Evaluate(const double E, const double dEdx, const double kB) const {
  double rv = 1.0 / (1 + kB * dEdx);
  return rv;
}
