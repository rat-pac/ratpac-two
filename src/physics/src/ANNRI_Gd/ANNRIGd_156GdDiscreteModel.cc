/**
 * @brief  Implementations for the ANNRIGd_156GdDiscreteModel class.
 * @author Sebastian Lorenz
 * @date   2017-08-06
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_156GdDiscreteModel.hh"

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
// extern int NumGamma;
// extern double GammaEnergies[15];

//==============================================================================
// CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructor.
 * @details Sets '156GdDiscreteModel' as model name.
 */
ANNRIGd_156GdDiscreteModel::ANNRIGd_156GdDiscreteModel()
    : ANNRIGd_Model("156GdDiscreteModel", ANNRIGd_ModelType::Mdl156GdDiscrete) {
  cout << "ANNRIGd_156GdDiscreteModel : Model initialized." << endl;
}

//______________________________________________________________________________
//! @brief Destructor.
ANNRIGd_156GdDiscreteModel::~ANNRIGd_156GdDiscreteModel() { /* Nothing done here */
}

//------------------------------------------------------------------------------
// PRIVATE METHODS

//______________________________________________________________________________
/**
 * @brief   Generates gamma-rays from the discrete peaks of 156Gd* after the
 *          thermal 155Gd(n,g) reaction.
 * @details There are five hard coded sequences of gamma-ray energies that can
 *          be generated. The selection is done by comparing a random number
 *          from ]0,1[ to the probabilities of the different branches.
 *          All generated gamma-rays have random directions.
 * @return  Vector containing the ReactionProduct objects for gamma-rays with
 *          random directions and energies in MeV corresponding to the
 *          discrete peaks spectrum part from the thermal 155Gd(n,g) reaction.
 */
ReactionProductVector ANNRIGd_156GdDiscreteModel::DoGenerate() const {
  // define relative intensities of discrete transition sequences;
  // ordered by decreasing intensity
  const double p1 = 0.0064;
  const double p2 = 0.0838;
  const double sp2 = p1 + p2;
  const double p3 = 0.1628;
  const double sp3 = sp2 + p3;
  const double p4 = 0.1266;
  const double sp4 = sp3 + p4;
  const double p5 = 0.1163;
  const double sp5 = sp4 + p5;
  const double p6 = 0.1088;
  const double sp6 = sp5 + p6;
  const double p7 = 0.0338;
  const double sp7 = sp6 + p7;
  const double p8 = 0.0733;
  const double sp8 = sp7 + p8;
  const double p9 = 0.0627;
  const double sp9 = sp8 + p9;
  const double p10 = 0.0674;
  const double sp10 = sp9 + p10;
  const double p11 = 0.1029;
  const double sp11 = sp10 + p11;
  //	const double p12 = 0.0552;  const double sp12 = sp11 + p12;

  ReactionProductVector products;  // storage for reaction products

  // find the discrete transition sequence to generate
  const double rndm = Rnd::Uniform();
  if (rndm < p1)
    Fill_156Gd_Discrete01(products);
  else if (rndm < sp2)
    Fill_156Gd_Discrete02(products);
  else if (rndm < sp3)
    Fill_156Gd_Discrete03(products);
  else if (rndm < sp4)
    Fill_156Gd_Discrete04(products);
  else if (rndm < sp5)
    Fill_156Gd_Discrete05(products);
  else if (rndm < sp6)
    Fill_156Gd_Discrete06(products);
  else if (rndm < sp7)
    Fill_156Gd_Discrete07(products);
  else if (rndm < sp8)
    Fill_156Gd_Discrete08(products);
  else if (rndm < sp9)
    Fill_156Gd_Discrete09(products);
  else if (rndm < sp10)
    Fill_156Gd_Discrete10(products);
  else if (rndm < sp11)
    Fill_156Gd_Discrete11(products);
  else
    Fill_156Gd_Discrete12(products);

  return products;
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          8.448 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete01(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 8.448));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 0.089));  // 2nd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          7.382 -> 1.154
 *        or
 *          7.382 -> 1.065 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete02(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 7.382));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  if (rndm < 0.545) {
    energies.push_back(ParticleEnergy(22, 1.154));  // 2nd gamma ray
  } else {
    energies.push_back(ParticleEnergy(22, 1.065));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          7.288 -> 1.158 -> 0.089
 *        or
 *          7.288 -> 0.959 -> 0.199 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete03(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 7.288));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  if (rndm < 0.768) {
    energies.push_back(ParticleEnergy(22, 1.158));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
  } else {
    energies.push_back(ParticleEnergy(22, 0.959));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.199));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 4th gamma ray
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.474 -> 1.964 -> 0.098
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete04(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.474));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 1.964));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.098));  // 3rd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.430 -> 2.017 -> 0.089
 *        or
 *          6.430 -> 1.818 -> 0.199 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete05(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.430));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  if (rndm < 0.639) {
    energies.push_back(ParticleEnergy(22, 2.017));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
  } else {
    energies.push_back(ParticleEnergy(22, 1.818));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.199));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 4th gamma ray
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.348 -> 2.188
 *        or
 *          6.348 -> 2.097 -> 0.089
 *        or
 *          6.348 -> 1.036 -> 1.154
 *        or
 *          6.348 -> 1.036 -> 1.065 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete06(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.348));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  const double rndm1 = G4UniformRand();
  if (rndm < 0.399) {
    energies.push_back(ParticleEnergy(22, 2.188));  // 2nd gamma ray
  } else if (rndm < 0.724) {
    energies.push_back(ParticleEnergy(22, 2.097));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray

  } else {
    energies.push_back(ParticleEnergy(22, 1.036));  // 2nd gamma ray
    if (rndm1 < 0.545) {
      energies.push_back(ParticleEnergy(22, 1.154));  // 2nd gamma ray
    } else {
      energies.push_back(ParticleEnergy(22, 1.065));  // 2nd gamma ray
      energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
    }
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.319 -> 2.127 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete07(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.319));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.127));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          6.034 -> 2.412 -> 0.089
 *        or
 *          6.034 -> 2.213 -> 0.199 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete08(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 6.034));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  if (rndm < 0.686) {
    energies.push_back(ParticleEnergy(22, 2.412));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
  } else {
    energies.push_back(ParticleEnergy(22, 2.213));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.199));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 4th gamma ray
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.885 -> 2.563 -> 0.089
 *        or
 *          5.885 -> 2.364 -> 0.199 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete09(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 5.885));  // 1st gamma ray
  // decide the which transition branch is used
  const double rndm = G4UniformRand();
  if (rndm < 0.518) {
    energies.push_back(ParticleEnergy(22, 2.563));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray
  } else {
    energies.push_back(ParticleEnergy(22, 2.364));  // 2nd gamma ray
    energies.push_back(ParticleEnergy(22, 0.199));  // 3rd gamma ray
    energies.push_back(ParticleEnergy(22, 0.089));  // 4th gamma ray
  }

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.779 -> 2.672 -> 0.085
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete10(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 5.779));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.672));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.085));  // 3rd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.698 -> 2.749 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete11(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 5.698));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.749));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

//______________________________________________________________________________
/**
 * @brief Generates the following sequence of gamma-ray energies [MeV]:
 *          5.661 -> 2.786 -> 0.089
 *        All gamma-rays are created with random directions.
 * @param products  Vector into which the created reaction products shall be
 *        filled.
 */
void ANNRIGd_156GdDiscreteModel::Fill_156Gd_Discrete12(ReactionProductVector& products) const {
  ParticleEnergies energies;

  energies.push_back(ParticleEnergy(22, 5.661));  // 1st gamma ray
  energies.push_back(ParticleEnergy(22, 2.786));  // 2nd gamma ray
  energies.push_back(ParticleEnergy(22, 0.089));  // 3rd gamma ray

  // NumGamma = energies.size();
  // for(int i=0;i<NumGamma;i++) GammaEnergies[i] = energies[i].second;

  Aux::FillRndmDirProducts(products, energies);
}

} /* namespace ANNRIGdGammaSpecModel */
