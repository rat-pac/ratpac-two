////////////////////////////////////////////////////////////////////
/// \class Evaluateable
///
/// \brief Interface for functions of a single variable
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Base class exposing a method for function evaluation, which acts
///          as an interface for defining functions of a single variable.
///
////////////////////////////////////////////////////////////////////

#ifndef __Evaluateable__
#define __Evaluateable__

class Evaluateable {
 public:
  Evaluateable();
  double operator()(double x);
  long GetCallCount();
  void ResetCallCount();
  virtual double Evaluate(double x) = 0;

 private:
  long count;
};

#endif
