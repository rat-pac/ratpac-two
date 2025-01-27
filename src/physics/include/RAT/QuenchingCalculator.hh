////////////////////////////////////////////////////////////////////
/// \class QuenchingCalculator
///
/// \brief Interface for calculating quenched energy deposits
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Base class exposing method to calculate the quenched energy deposit
///          over a single tracking step.
///
////////////////////////////////////////////////////////////////////

#ifndef __QuenchingCalculator__
#define __QuenchingCalculator__

#include <G4Step.hh>
#include <RAT/BirksLaw.hh>
#include <RAT/Quadrature.hh>

class QuenchingCalculator {
 public:
  QuenchingCalculator(BirksLaw model);
  virtual ~QuenchingCalculator() = default;
  virtual double QuenchedEnergyDeposit(const G4Step& step, const double kB) = 0;

 protected:
  BirksLaw model;

 private:
  /**/
};

#endif
