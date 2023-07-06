/** \file AmBeSource.hh
 *  AmBeSource header file.
 *
 *  Author: Vincent Fischer
 */
#ifndef __RAT_AmBeSource__
#define __RAT_AmBeSource__

#include <CLHEP/Vector/LorentzVector.h>

#include <cmath>
#include <iostream>
#include <vector>

/** AmBeSource generates photons and neutrons from
 *  americium+beryllium interactions. Each
 *  neutron is given an energy by the AmBeNeutronSpectrum function,
 *  which the probability density of the produced neutrons as a
 *  function of neutron energy.  The photons are generated
 *  by the AmBeGammaSpectrum
 *  function with a probability found in the litterature.
 */

namespace RAT {

class AmBeSource {
 public:
  /** AmBeSource constructor.  Use a default reaction of AmBe,*/
  AmBeSource();

  /** AmBeSource destructor. */
  ~AmBeSource();

  /** AmBeSource copy constructor. */
  AmBeSource(const AmBeSource &AmBeSource);

  /** AmBeSource overloaded = operator */
  AmBeSource &operator=(const AmBeSource &rhs);

  /** Returns the neutron multiplicity */
  int GetNumNeutron() const { return Nneutron; }

  /** Returns the prompt photon multiplicity */
  int GetNumGamma() const { return Ngamma; }

  /** Returns the energy of the neutrons produced by each AmBe decay.
   *  Called with the integer index for each neutron in the neutronE
   *  array, from 0 to the total number of neutrons. */
  CLHEP::HepLorentzVector GetAmBeNeutronMomentum(int n) const { return neutronE[n]; }
  double GetAmBeNeutronTime(int n) const { return Tneutron[n]; }

  /** Returns the energy of the gammas produced by each AmBe decay.
   *  Called with the integer index for each gamma in the gammaE
   *  array, from 0 to the total number of gammas/source. */
  CLHEP::HepLorentzVector GetAmBeGammaMomentum(int n) const { return gammaE[n]; }
  double GetAmBeGammaTime(int n) const { return Tgamma[n]; }

 private:
  static const int maxNeutron = 1;
  static const int maxGamma = 1;
  int Nneutron, Ngamma;

  /** \var neutronE
   * Array containing the momentum of the neutrons from
   *  each AmBe decay, indexed from 0 to the total number of
   *  neutrons. */
  std::vector<CLHEP::HepLorentzVector> neutronE;
  std::vector<double> Tneutron;

  /** \var gammaE
   *  Array containing the momentum of the gammas from
   *  each AmBe decay, indexed from 0 to the total number of
   *  gammas. */
  std::vector<CLHEP::HepLorentzVector> gammaE;
  std::vector<double> Tgamma;

  // G4 particle definitions.
  static double massNeutron;

  /** The probability density of the prompt neutrons from the AmBe
   *  decay as a function of neutron energy. */
  static double AmBeNeutronSpectrum(const double &x);
};

}  // namespace RAT

#endif
