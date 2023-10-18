////////////////////////////////////////////////////////////////////
/// \class BirksLaw
///
/// \brief Interface to functional form of nonlinear component of Birks' law
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Class to evaluate nonlinear quenching component specified by
///          single-parameter Birks' law.
///
////////////////////////////////////////////////////////////////////

#ifndef __BirksLaw__
#define __BirksLaw__

class BirksLaw {
 public:
  BirksLaw(){/* */};
  double Evaluate(const double E, const double dEdx, const double kB) const;

 protected:
  /**/
 private:
  /**/
};

#endif
