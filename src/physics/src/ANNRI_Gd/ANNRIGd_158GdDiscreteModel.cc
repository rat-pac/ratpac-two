/**
 * @brief  Implementations for the ANNRIGd_158GdDiscreteModel class.
 * @author Sebastian Lorenz
 * @date   2017-08-06
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_158GdDiscreteModel.hh"

#include "RAT/ANNRIGd_Auxiliary.hh"
#include "RAT/ANNRIGd_Random.hh"
// STD includes
#include <iostream>

namespace Rnd = ANNRIGdGammaSpecModel::Random;
namespace Aux = ANNRIGdGammaSpecModel::Auxiliary;
using Aux::ParticleEnergies;
using Aux::ParticleEnergy;
using std::cout;
using std::endl;

extern std::ofstream outf;
extern int NumGamma;
extern double GammaEnergies[15];

//==============================================================================
// CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructor.
 * @details Sets '158GdDiscreteModel' as model name.
 */
ANNRIGd_158GdDiscreteModel::ANNRIGd_158GdDiscreteModel()
    : ANNRIGd_Model("158GdDiscreteModel", ANNRIGd_ModelType::Mdl158GdDiscrete) {
  cout << "ANNRIGd_158GdDiscreteModel : Model initialized." << endl;
}

//______________________________________________________________________________
//! @brief Destructor.
ANNRIGd_158GdDiscreteModel::~ANNRIGd_158GdDiscreteModel() { /* Nothing done here */
}

//------------------------------------------------------------------------------
// PRIVATE METHODS

//______________________________________________________________________________
/**
 * @brief   Generates gamma-rays from the discrete peaks of 158Gd* after the
 *          thermal 157Gd(n,g) reaction.
 * @details There are 15 hard coded sequences of gamma-ray energies that can
 *          be generated. The selection is done by comparing a random number
 *          from ]0,1[ to the measured probabilities of the different branches.
 *          All generated gamma-rays have random directions.
 * @return  Vector containing the ReactionProduct objects for gamma-rays with
 *          random directions and energies in MeV corresponding to the
 *          discrete peaks spectrum part from the thermal 157Gd(n,g) reaction.
 */
ReactionProductVector ANNRIGd_158GdDiscreteModel::DoGenerate() const {
  // define relative intensities of discrete transition sequences;
  // ordered by decreasing intensity
  const double p1 = 0.3476;
  const double p2 = 0.1694;
  const double sp2 = p1 + p2;
  const double p3 = 0.0961;
  const double sp3 = sp2 + p3;
  const double p4 = 0.0913;
  const double sp4 = sp3 + p4;
  const double p5 = 0.0869;
  const double sp5 = sp4 + p5;
  const double p6 = 0.0470;
  const double sp6 = sp5 + p6;
  const double p7 = 0.0343;
  const double sp7 = sp6 + p7;
  const double p8 = 0.0285;
  const double sp8 = sp7 + p8;
  const double p9 = 0.0274;
  const double sp9 = sp8 + p9;
  const double p10 = 0.0234;
  const double sp10 = sp9 + p10;
  const double p11 = 0.0227;
  const double sp11 = sp10 + p11;
  const double p12 = 0.0182;
  const double sp12 = sp11 + p12;
  const double p13 = 0.0034;
  const double sp13 = sp12 + p13;
  const double p14 = 0.0030;
  const double sp14 = sp13 + p14;
  //	const double p15 = 0.0008;

  ReactionProductVector products;  // storage for reaction products

  // find the discrete transition sequence to generate
  const double rndm = Rnd::Uniform();
  if (rndm < p1)
    Fill_158Gd_Discrete01(products);
  else if (rndm < sp2)
    Fill_158Gd_Discrete02(products);
  else if (rndm < sp3)
    Fill_158Gd_Discrete03(products);
  else if (rndm < sp4)
    Fill_158Gd_Discrete04(products);
  else if (rndm < sp5)
    Fill_158Gd_Discrete05(products);
  else if (rndm < sp6)
    Fill_158Gd_Discrete06(products);
  else if (rndm < sp7)
    Fill_158Gd_Discrete07(products);
  else if (rndm < sp8)
    Fill_158Gd_Discrete08(products);
  else if (rndm < sp9)
    Fill_158Gd_Discrete09(products);
  else if (rndm < sp10)
    Fill_158Gd_Discrete10(products);
  else if (rndm < sp11)
    Fill_158Gd_Discrete11(products);
  else if (rndm < sp12)
    Fill_158Gd_Discrete12(products);
  else if (rndm < sp13)
    Fill_158Gd_Discrete13(products);
  else if (rndm < sp14)
    Fill_158Gd_Discrete14(products);
  else
    Fill_158Gd_Discrete15(products);

  return products;
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.750 -> 1.187
 *        or
 *          6.750 -> 1.107 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete01(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.750));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = Rnd::Uniform();
  if (rndm < 0.501) {                               // 1.0 branch
    energies.push_back(ParticleEnergy(22, 1.187));  // 2nd gamma ray
  } else {                                          // 2.0 branch
    energies.push_back(ParticleEnergy(22, 1.107));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
  }
  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.903 -> 1.010 -> 0.994 -> 0.080
 *        or
 *          5.903 -> 0.875 -> 0.898 -> 0.182 -> 0.080
 *        or
 *          5.903 -> 0.769 -> 1.186 -> 0.080
 *        or
 *          5.903 -> 0.769 -> 1.004 -> 0.182 -> 0.080
 *        or
 *          5.903 -> 0.676 -> 1.097 -> 0.128 -> 0.080
 *        or
 *          5.903 -> 0.676 -> 1.279 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete02(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 5.903));  // 1st gamma ray
  // decide the which transition branch is used
  // branching ratios
  const double br1 = 0.393;
  const double br2 = 0.254;
  const double br3 = 0.222;
  // const double br4 = 0.134;

  const double rndm = Rnd::Uniform();
  if (rndm < br1) {                                 // 1.0 branch
    energies.push_back(ParticleEnergy(22, 1.010));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.944));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
  } else if (rndm < br1 + br2) {                    // 2.0 branch
    energies.push_back(ParticleEnergy(22, 0.875));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.898));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.182));  // 4th gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 5th gamma ray
  } else if (rndm < br1 + br2 + br3) {              // 3.0 branch
    energies.push_back(ParticleEnergy(22, 0.769));  // 2nd gamma ray
    // decide the which transition branch is used
    const double rndm2 = Rnd::Uniform();
    if (rndm2 < 0.847) {                              // 3.1 branch
      energies.push_back(ParticleEnergy(22, 1.186));  // 3rd gamma ray
      energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
    } else {                                          // 3.2 branch
      energies.push_back(ParticleEnergy(22, 1.004));  // 3rd gamma ray
      energies.push_back(ParticleEnergy(22, 0.182));  // 4th gamma ray
      energies.push_back(ParticleEnergy(22, 0.080));  // 5th gamma ray
    }
  } else {                                          // 4.0 branch
    energies.push_back(ParticleEnergy(22, 0.676));  // 2nd gamma ray
    // decide the which transition branch is used
    const double rndm2 = Rnd::Uniform();
    if (rndm2 < 0.755) {                              // 4.1 branch
      energies.push_back(ParticleEnergy(22, 1.097));  // 3rd gamma ray
      energies.push_back(ParticleEnergy(22, 0.182));  // 4th gamma ray
      energies.push_back(ParticleEnergy(22, 0.080));  // 5th gamma ray
    } else {                                          // 4.2 branch
      energies.push_back(ParticleEnergy(22, 1.279));  // 3rd gamma ray
      energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
    }
  }

  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.595 -> 2.262 -> 0.080
 *        Covers the three peaks at 5.610, 5.593 and 5.582.
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete03(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.595));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.262));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2342 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.669 -> 2.188 -> 0.080
 *        Covers the two peaks at 5.677 and 5.661.
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete04(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.669));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.188));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2268 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.167 -> 2.690 -> 0.080
 *        Covers the two peaks at 5.179 and 5.155.
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete05(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.167));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.690));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2270 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.420 -> 1.517
 *        or
 *          6.420 -> 1.438 -> 0.080
 *        or
 *          6.420 -> 1.256 -> 0.182 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete06(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.420));  // 1st gamma ray
  // decide the which transition branch is used
  // branching ratios
  const double br1 = 0.381;
  const double br2 = 0.416;
  // const double br3 = 0.203;

  const double rndm = Rnd::Uniform();
  if (rndm < br1) {                                 // 1.0 branch
    energies.push_back(ParticleEnergy(22, 1.517));  // 2nd gamma ray
  } else if (rndm < br1 + br2) {                    // 2.0 branch
    energies.push_back(ParticleEnergy(22, 1.438));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
  } else {                                          // 3.0 branch
    energies.push_back(ParticleEnergy(22, 1.256));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.182));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
  }

  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.543 -> 2.314 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete07(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.543));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.314));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2394 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.784 -> 2.073 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete08(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.784));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.073));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2153 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.672 -> 1.186
 *        or
 *          6.672 -> 1.004 -> 0.182 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete09(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.672));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = Rnd::Uniform();
  if (rndm < 0.847) {                               // 1.0 branch
    energies.push_back(ParticleEnergy(22, 1.186));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                    // TODO VIOLATES Q-VALUE BY -79 keV
  } else {                                          // 2.0 branch
    energies.push_back(ParticleEnergy(22, 1.004));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.182));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
                                                    // TODO VIOLATES Q-VALUE BY 1 keV
  }

  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.436 -> 2.421 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete10(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 5.436));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.421));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -2501 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.001 -> 0.769 -> 1.187
 *        or
 *          6.001 -> 0.769 -> 1.107 -> 0.080
 *        Covers the two peaks at 6.006 and 5.995.
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete11(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 6.001));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 0.769));  // 2nd gamma ray
  // decide the which transition branch is used
  const double rndm = Rnd::Uniform();
  if (rndm < 0.501) {                               // 1.0 branch
    energies.push_back(ParticleEnergy(22, 1.187));  // 3rd gamma ray
  } else {                                          // 2.0 branch
    energies.push_back(ParticleEnergy(22, 1.007));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.080));  // 4th gamma ray
  }

  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -1936 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.914 -> 0.944 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete12(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 6.914));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 0.944));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 3rd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY 1 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          7.857 -> 0.080
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete13(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 7.857));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 0.080));  // 2nd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.960 -> 0.977
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete14(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 6.960));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 0.977));  // 2nd gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
  // TODO VIOLATES Q-VALUE BY -977 keV
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          7937 (Q-value)
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_158GdDiscreteModel::Fill_158Gd_Discrete15(ReactionProductVector& products) const {
  ParticleEnergies energies;
  energies.push_back(ParticleEnergy(22, 7.937));  // 1st gamma ray
                                                  // save gamma multiplicity
  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;
  Aux::FillRndmDirProducts(products, energies);
}

} /* namespace ANNRIGdGammaSpecModel */
