/**
 * @brief  Definition and declaration of auxiliary types and functions to be
 *         used in the ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-07-31
 */

#ifndef ANNRIGD_AUXILIARY_HH_
#define ANNRIGD_AUXILIARY_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ReactionProduct.hh"
// STD includes
#include <utility>
#include <vector>

//==============================================================================
// AUXILIARY DECLARATIONS AND DEFINITIONS

namespace ANNRIGdGammaSpecModel {
namespace Auxiliary {

//______________________________________________________________________________
// struct definitions

//------------------------------------------------------------------------------
/**
 * @brief Container class of the three components of a 3D direction in
 *        Euclidean space.
 */
struct Direction {
  double x_;  //!< x-component
  double y_;  //!< y-component
  double z_;  //!< z-component
};

//______________________________________________________________________________
// type definitions

//! @brief Definition of the 'ParticleEnergy' type as a combination of the
//!        particle's PDG ID (first) and the kinetic energy in MeV (second).
typedef std::pair<int, double> ParticleEnergy;

//! @brief Definition of the 'ParticleEnergies' container type as a vector of
//!        for ParticleEnergy objects.
typedef std::vector<ParticleEnergy> ParticleEnergies;

//______________________________________________________________________________
// function declarations

void FillRndmDirProducts(ReactionProductVector& products, const ParticleEnergies& energies);

Direction GenerateRndmDir();

}  // namespace Auxiliary
}  // namespace ANNRIGdGammaSpecModel

#endif /* ANNRIGD_AUXILIARY_HH_ */
