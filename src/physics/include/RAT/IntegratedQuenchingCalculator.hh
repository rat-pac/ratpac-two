////////////////////////////////////////////////////////////////////
/// \class IntegratedQuenchingCalculator
///
/// \brief Interface for properly integrating quenching model over track steps
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Base class exposing method for calculating integrated quenched
///          energy deposit over a track step.
///
////////////////////////////////////////////////////////////////////

#ifndef __IntegratedQuenchingCalculator__
#define __IntegratedQuenchingCalculator__

#include <RAT/QuenchingCalculator.hh>

class IntegratedQuenchingCalculator : public QuenchingCalculator {
 public:
  IntegratedQuenchingCalculator(BirksLaw model, Quadrature* quadrature);
  virtual double QuenchedEnergyDeposit(const G4Step& step, const double kB);

 protected:
  /**/
 private:
  Quadrature* fQuadrature;
};

#endif
