/**
 * @brief  Implementations of auxiliary functions associated with the
 *         ANNRIGd_ModelType::ID enum.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ModelType.hh"

//==============================================================================
// FUNCTION IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Returns for the given ANNRIGd_ModelType::ID enum value a
 *          corresponding string.
 * @details If the given enum value is unknown, the string '-UNKNOWN-' is
 *          returned.
 * @param   mdlID Enum value to return the corresponding string for.
 * @post    Returned string is not empty.
 * @return  String corresponding to given ANNRIGd_ModelType::ID enum value.
 */

std::string ANNRIGd_ModelType::ToString(ANNRIGd_ModelType::ID mdlID) {
  switch (mdlID) {
    case ANNRIGd_ModelType::MdlDummy:
      return "dummy model";
      break;
    case ANNRIGd_ModelType::Mdl156GdContinuum:
      return "156Gd continuum model";
      break;
    case ANNRIGd_ModelType::Mdl156GdDiscrete:
      return "156Gd discrete peaks model";
      break;
    case ANNRIGd_ModelType::Mdl158GdContinuum:
      return "158Gd continuum model";
      break;
    case ANNRIGd_ModelType::Mdl158GdDiscrete:
      return "158Gd discrete peaks model";
      break;
    default:
      return "-UNKONWN-";
      break;
  }
}

} /* namespace ANNRIGdGammaSpecModel */
