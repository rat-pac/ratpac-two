/**
 * @brief  Definition of the ReactionProduct struct and containers using it in
 *         the ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-07-31.
 */
#ifndef ANNRIGD_REACTIONPRODUCT_HH_
#define ANNRIGD_REACTIONPRODUCT_HH_

//==============================================================================
// INCLUDES

// STD includes
#include <vector>

//==============================================================================
// STRUCT AND FURTHER DEFINITIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief Container for basic information on a product from the disexcitation
 *        of gadolinium (gamma-rays, IC electrons) after the thermal Gd(n,g)
 *        reaction.
 */
struct ReactionProduct {
  double eTot_;  //!< total energy [MeV] in nucleus rest frame
  double px_;    //!< x-momentum [MeV/c] in nucleus rest frame
  double py_;    //!< y-momentum [MeV/c] in nucleus rest frame
  double pz_;    //!< z-momentum [MeV/c] in nucleus rest frame
  int pdgID_;    //!< PDG ID

  ReactionProduct(int pdgId, double eTot, double px, double py, double pz);
};

//! Definition of ReactionProductVector type as std::vector of ReactionProduct
//! objects.
typedef std::vector<ReactionProduct> ReactionProductVector;

}  // namespace ANNRIGdGammaSpecModel

#endif /* ANNRIGD_REACTIONPRODUCT_HH_ */
