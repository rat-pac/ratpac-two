////////////////////////////////////////////////////////////////////
/// \class AdaptiveSimpsonQuadrature
///
/// \brief Integral estimation by recursive application of Simpson's rule
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Recursively apply quadratic approximations as an adaptive
///          quadrature rule.
///
////////////////////////////////////////////////////////////////////

#ifndef __AdaptiveSimpsonQuadrature__
#define __AdaptiveSimpsonQuadrature__

#include <RAT/Quadrature.hh>

class AdaptiveSimpsonQuadrature : public Quadrature {
 public:
  AdaptiveSimpsonQuadrature(){/**/};
  ~AdaptiveSimpsonQuadrature(){/**/};
  AdaptiveSimpsonQuadrature(double _tolerance);
  virtual double Integrate(Evaluateable& f, const double xlo, const double xhi);

 protected:
  double fTolerance;
  virtual double evaluate(Evaluateable& f, double xlo, double& flo, double& xmd, double& fmd, double xhi, double& fhi);
  double refine(Evaluateable& f, double whole, double xlo, double flo, double xmd, double fmd, double xhi, double fhi);

 private:
  /**/
};

#endif
