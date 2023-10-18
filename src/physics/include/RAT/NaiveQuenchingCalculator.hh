////////////////////////////////////////////////////////////////////
/// \class NaiveQuenchingCalculator
///
/// \brief Default GLG4Scint quenching approximation
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Evaluate quenching by approximating an average stopping power over
///          a full track step. WARNING: THIS IS HEAVILY DEPENDENT ON STEP SIZES
///          USED BY GEANT4! BEST PRACTICE IS TO USE THE INTEGRATED METHOD!
///
////////////////////////////////////////////////////////////////////

#ifndef __NaiveQuenchingCalculator__
#define __NaiveQuenchingCalculator__

#include <RAT/QuenchingCalculator.hh>

class NaiveQuenchingCalculator : public QuenchingCalculator {
 public:
  NaiveQuenchingCalculator(BirksLaw model);
  virtual double QuenchedEnergyDeposit(const G4Step& step, const double kB);

 protected:
  /**/
 private:
  /**/
};

#endif
