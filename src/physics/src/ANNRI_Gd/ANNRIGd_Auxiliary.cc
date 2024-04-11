/**
 * @brief  Implementation of auxiliary functions to be used in the ANNRI-Gd
 *         generator code.
 * @author Sebastian Lorenz
 * @date   2017-07-31
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_Auxiliary.hh"

#include "RAT/ANNRIGd_Random.hh"
// STD includes
#include <cmath>

namespace Rnd = ANNRIGdGammaSpecModel::Random;
using ANNRIGdGammaSpecModel::Auxiliary::Direction;

//==============================================================================
// IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Fills the given ReactionProductVector instance with ReactionProduct
 *          objects that are created from the given ParticleEnergy objects within
 *          the given ParticleEnergies container.
 * @details The ReactionProduct objects are created with random 3D directions
 *          and the total energy is calculated based on the PDG ID stored in the
 *          given corresponding ParticleEnergy object (take mass of electrons
 *          from IC into account!).
 * @param   products  Vector to fill the created ReactionProduct instances
 *          into.
 * @param   energies  ParticleEnegies container holding the ParticleEnergy
 *          objects for which the ReactionProduct objects with random directions
 *          shall be created.
 */
void Auxiliary::FillRndmDirProducts(ReactionProductVector& products, const ParticleEnergies& energies) {
  for (ParticleEnergies::const_iterator iEnergies = energies.begin(), iEnd = energies.end(); iEnergies not_eq iEnd;
       ++iEnergies) {
    const int pdgId = (*iEnergies).first;
    const double eKin = (*iEnergies).second;
    const Direction momDir = GenerateRndmDir();

    double eTot = eKin;
    double p = eTot;

    // handle different particles
    switch (pdgId) {
      case 22:  // handle photons - everything is fine
        break;
      case 11:  // handle (conversion) electrons
        eTot = eKin + 0.511;
        p = sqrt(eTot * eTot - 0.511 * 0.511);
        break;
      default:  // do not fill information
        continue;
        break;
    }

    products.push_back(ReactionProduct(pdgId, eTot, p * momDir.x_, p * momDir.y_, p * momDir.z_));
  }
}

//______________________________________________________________________________
/**
 * @brief  Computes a random 3D direction in Euclidean space.
 * @return Direction object containing the 3D coordinates of a random direction
 *         in Euclidean space. The magnitude of the returned vector is 1.
 */
Direction Auxiliary::GenerateRndmDir() {
  const double costheta = 2.0 * Rnd::Uniform() - 1;
  const double theta = acos(costheta);
  const double sinth = sin(theta);
  const double phi = 6.28318530717958623 * G4UniformRand();

  Direction dir;
  dir.x_ = sinth * cos(phi);
  dir.y_ = sinth * sin(phi);
  dir.z_ = costheta;

  return dir;
}

}  // namespace ANNRIGdGammaSpecModel
