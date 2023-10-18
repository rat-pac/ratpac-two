////////////////////////////////////////////////////////////////////
/// \class FixedTrapezoidalQuadrature
///
/// \brief Trapezoidal quadrature rule applied with a uniform grid spacing
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details The well-known trapezoidal rule applied to a uniformly spaced grid.
///
////////////////////////////////////////////////////////////////////

#ifndef __FixedTrapezoidalQuadrature__
#define __FixedTrapezoidalQuadrature__

#include <RAT/Quadrature.hh>

class FixedTrapezoidalQuadrature : public Quadrature {
 public:
  FixedTrapezoidalQuadrature(){/**/};
  FixedTrapezoidalQuadrature(double);
  virtual double Integrate(Evaluateable& f, const double xlo, const double xhi);

 protected:
  double fSpacing;

 private:
  /**/
};

#endif
