/**
 * @brief  Definition of the ANNRIGd_ModelType::ID enum and declaration of
 *         associated auxiliary functions used in the ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

#ifndef ANNRIGD_MODELTYPE_HH_
#define ANNRIGD_MODELTYPE_HH_

//==============================================================================
// INCLUDES

// STD includes
#include <string>

//==============================================================================
// ENUM DEFINITION AND AUXILIARY FUNCTION DECLARATIONS

namespace ANNRIGdGammaSpecModel {
namespace ANNRIGd_ModelType {

//______________________________________________________________________________
//! @enum ID
//! @brief Identifiers for model types.
enum ID {
  MdlDummy,           //!< dummy model
  Mdl156GdContinuum,  //!< 156Gd continuum model
  Mdl156GdDiscrete,   //!< 156Gd discrete peaks model
  Mdl158GdContinuum,  //!< 158Gd continuum model
  Mdl158GdDiscrete    //!< 158Gd discrete peaks model
};

bool IsKnown(ANNRIGd_ModelType::ID mdlID);
std::string ToString(ANNRIGd_ModelType::ID mdlID);

}  // namespace ANNRIGd_ModelType

//==============================================================================
// INLINE AUXILIARY FUNCTION IMPLEMENTATIONS

//______________________________________________________________________________
//! @brief   Checks if the given model ID is known.
//! @details It is checked if the given model ID is larger or equal to the ID
//!          of 'MdlDummy' and smaller or equal to 'Mdl158GdDiscrete'.
//! @return  True, if the given model ID is known. Otherwise false.
inline bool ANNRIGd_ModelType::IsKnown(ANNRIGd_ModelType::ID mdlID) {
  return mdlID >= MdlDummy and mdlID <= Mdl158GdDiscrete;
}

} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_MODELTYPE_HH_ */
