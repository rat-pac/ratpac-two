////////////////////////////////////////////////////////////////////
/// \class BirksLaw
///
/// \brief Interface to evaluate quenching model for a given particle/material
///
/// \author Ed Callaghan <ejc3@berkeley.edu>
///
/// REVISION HISTORY:\n
///  - 2022-09 : Ed Callaghan - First revision
///
/// \details Interface for evaluating a given quenching model for a given
///          particle traversing a given material, adhering to interface
///          necessary for e.g. integration methods. Currently, only single
///          -parameter BirksLaw is supported as a model, but this can be
///          abstracted into a generic quenching model.
///
////////////////////////////////////////////////////////////////////

#ifndef __AppliedQuenchingModel__
#define __AppliedQuenchingModel__

#include <G4Material.hh>
#include <G4ParticleDefinition.hh>
#include <RAT/BirksLaw.hh>
#include <RAT/EnergyLossFunction.hh>
#include <RAT/Evaluateable.hh>
#include <memory>

class AppliedQuenchingModel : public Evaluateable {
 public:
  AppliedQuenchingModel();
  AppliedQuenchingModel(const BirksLaw _model, const G4ParticleDefinition* def, const G4Material* mat,
                        const double _kB);
  double Evaluate(double);

 protected:
  /**/
 private:
  const BirksLaw model;  // TODO abstract to reference to general model
  const double kB;       // TODO abstract to vector/array of parameters
  std::unique_ptr<EnergyLossFunction> loss;
};

#endif
