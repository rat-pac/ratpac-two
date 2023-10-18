#include <RAT/AdaptiveSimpsonQuadrature.hh>
#include <RAT/Log.hh>
#include <cmath>

AdaptiveSimpsonQuadrature::AdaptiveSimpsonQuadrature(double _tolerance) : fTolerance(_tolerance) { /* */ }

double AdaptiveSimpsonQuadrature::Integrate(Evaluateable& f, double xlo, double xhi) {
  double xmd, fmd;
  double flo, fhi;
  flo = f(xlo);
  fhi = f(xhi);
  double whole = this->evaluate(f, xlo, flo, xmd, fmd, xhi, fhi);
  double rv = this->refine(f, whole, xlo, flo, xmd, fmd, xhi, fhi);
  return rv;
}

double AdaptiveSimpsonQuadrature::evaluate(Evaluateable& f, double xlo, double& flo, double& xmd, double& fmd,
                                           double xhi, double& fhi) {
  xmd = 0.5 * (xlo + xhi);
  fmd = f(xmd);
  double rv = (flo + 4.0 * fmd + fhi) * (xhi - xlo) / 6.0;
  return rv;
}

double AdaptiveSimpsonQuadrature::refine(Evaluateable& f, double whole, double xlo, double flo, double xmd, double fmd,
                                         double xhi, double fhi) {
  double rv;
  double lxmd, lfmd;
  double rxmd, rfmd;
  double left = this->evaluate(f, xlo, flo, lxmd, lfmd, xmd, fmd);
  double right = this->evaluate(f, xmd, fmd, rxmd, rfmd, xhi, fhi);
  double divided = left + right;
  double error = fabs(divided - whole) / whole;
  if (error < fTolerance || std::isnan(error)) {
    rv = divided;
    if (std::isnan(error)) {
      RAT::warn << "AdaptiveSimpsonQuadrature::refine: Warning! Encountered nan and halting integration refinement."
                << newline;
    }
  } else {
    rv = 0.0;
    rv += this->refine(f, left, xlo, flo, lxmd, lfmd, xmd, fmd);
    rv += this->refine(f, right, xmd, fmd, rxmd, rfmd, xhi, fhi);
  }
  return rv;
}
