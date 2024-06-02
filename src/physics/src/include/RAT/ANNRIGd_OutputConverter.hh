/**
 * @brief  Declaration of functions to convert the output of the
 *         ANNRI-Gd generator to other formats, e.g., a format used by Geant4.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

#ifndef ANNRIGD_OUTPUTCONVERTER_HH_
#define ANNRIGD_OUTPUTCONVERTER_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ReactionProduct.hh"
// Geant4 includes
#include "G4ReactionProductVector.hh"

//==============================================================================
// FUNCTION DECLARATIONS

namespace ANNRIGdGammaSpecModel {
namespace ANNRIGd_OutputConverter {

G4ReactionProductVector* ConvertToG4(const ReactionProductVector& products);

} /* namespace ANNRIGd_OutputConverter */
} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_OUTPUTCONVERTER_HH_ */
