////////////////////////////////////////////////////////////////////
/// \class Quadrature
///
/// \brief Interface for algorithms to integrate functions of a single variable
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Base class exposing a method for function integration, which acts
///          as an interface for different quadrature rules to adhere to.
///
////////////////////////////////////////////////////////////////////

#ifndef __Quadrature__
#define __Quadrature__

#include <RAT/Evaluateable.hh>

class Quadrature {
 public:
  virtual ~Quadrature() = 0;
  virtual double Integrate(Evaluateable& f, const double xlo, const double xhi) = 0;

 protected:
  /**/
 private:
  /**/
};

#endif
