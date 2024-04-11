/**
 * @brief  Implementation of functions from the ANNRIGd_OutputConverter
 *         namespace.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_OutputConverter.hh"
// Geant4 includes
#include "G4Electron.hh"
#include "G4Gamma.hh"
#include "G4Geantino.hh"
#include "G4ReactionProductVector.hh"

//==============================================================================
// FUNCTION IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief  Converts the given ReactionProduct objects from the
 *         ANNRIGdGammaSpecModel namespace to G4ReactionProduct objects used by
 *         Geant4 and returns them in a G4ReactionProductVector instance.
 * @param  products  ReactionProductVector instance that contains ReactionProduct
 *         objects that shall be converted to G4ReactionProduct objects.
 * @post   Returned raw pointer is not NULL.
 * @return Raw pointer to G4ReactionProductVector containing the
 *         G4ReactionProduct objects created in the conversion. The caller is
 *         responsible for handling the lifetime of the underlying object!
 */
G4ReactionProductVector* ANNRIGd_OutputConverter::ConvertToG4(const ReactionProductVector& products) {
  G4ReactionProductVector* g4products = new G4ReactionProductVector();

  for (ReactionProductVector::const_iterator iProduct = products.begin(), iEnd = products.end(); iProduct not_eq iEnd;
       ++iProduct) {
    G4ReactionProduct* g4product = new G4ReactionProduct();
    switch (iProduct->pdgID_) {
      case 22:  // gamma-rays
        g4product->SetDefinition(G4Gamma::Gamma());
        break;
      case 11:  // (IC) electrons
      default:
        g4product->SetDefinition(G4Electron::Electron());
        break;
    }
    g4product->SetTotalEnergy(iProduct->eTot_);
    g4product->SetMomentum(G4ThreeVector(iProduct->px_, iProduct->py_, iProduct->pz_));
    g4products->push_back(g4product);
  }

  return g4products;
}

} /* namespace ANNRIGdGammaSpecModel */
