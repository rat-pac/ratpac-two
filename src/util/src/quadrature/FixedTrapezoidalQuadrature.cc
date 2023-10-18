#include <RAT/FixedTrapezoidalQuadrature.hh>

FixedTrapezoidalQuadrature::FixedTrapezoidalQuadrature(double _spacing) : fSpacing(_spacing) { /**/
}

double FixedTrapezoidalQuadrature::Integrate(Evaluateable& f, const double xlo, const double xhi) {
  double rv = 0.0;
  double x;

  // sum inner (double-counted) evaluations
  for (x = xlo + fSpacing; x < xhi - fSpacing; x += fSpacing) {
    rv += f(x);
  }

  // account for first and last points
  rv += 0.5 * (f(xlo) + f(x));

  // account for grid spacing
  rv *= this->fSpacing;

  return rv;
}
