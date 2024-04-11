/**
 * @brief  Implementations for the ReactionProduct struct.
 * @author Sebastian Lorenz
 * @date   2017-07-31.
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ReactionProduct.hh"

//==============================================================================
// STRUCT METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief Constructor with parameters.
 * @param pdgId  PDG ID of the particle being a reaction product.
 * @param eTot  Total energy (mass + kin. energy) of the product in MeV.
 * @param px  x-component of the particle's three-momentum in MeV/c.
 * @param py  y-component of the particle's three-momentum in MeV/c.
 * @param pz  z-component of the particle's three-momentum in MeV.
 */
ReactionProduct::ReactionProduct(int pdgId, double eTot, double px, double py, double pz)
    : eTot_(eTot), px_(px), py_(py), pz_(pz), pdgID_(pdgId) { /* Nothing done. */
}

}  // namespace ANNRIGdGammaSpecModel
