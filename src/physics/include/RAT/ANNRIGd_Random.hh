/**
 * @brief   Declaration and inline implementation of functions for random number
 *          generation within the ANNRI-Gd generator code.
 * @author  Sebastian Lorenz
 * @date    2017-07-31
 */

#ifndef ANNRIGD_RANDOM_HH_
#define ANNRIGD_RANDOM_HH_

//==============================================================================
// INCLUDES

// Geant4 / CLHEP includes
#include "Randomize.hh"

//==============================================================================
// FUNCTION DECLARATIONS AND INLINE IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {
namespace Random {

double Uniform();

}  // namespace Random

//______________________________________________________________________________
/**
 * @brief   Uniformly distributed random number generation.
 * @details Uses functionality of Geant4 / CLHEP.
 * @return  Random number from the interval ]0,1[.
 */
inline double Random::Uniform() { return G4UniformRand(); }

}  // namespace ANNRIGdGammaSpecModel

#endif /* ANNRIGD_RANDOM_HH_ */
