/**
 * @brief  Definition of the ANNRIGd_DummyModel class used in the
 * 		   ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_DummyModel.hh"

#include <iostream>
using std::cout;
using std::endl;

//==============================================================================
// CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructors.
 * @details Sets 'DummyModel' as model name.
 */
ANNRIGd_DummyModel::ANNRIGd_DummyModel()
    : ANNRIGd_Model("DummyModel", ANNRIGd_ModelType::MdlDummy) { /* Nothing done here */
}

//______________________________________________________________________________
//! @brief Destructor
ANNRIGd_DummyModel::~ANNRIGd_DummyModel() { /* Nothing done here */
}

//------------------------------------------------------------------------------
// PRIVATE METHODS

//______________________________________________________________________________
/**
 * @brief   Returns empty ReactionProductVector instance.
 * @details The user is warned about the try to generate reaction products with
 *          the dummy model.
 * @return  Empty ReactionProductVector instance.
 */
ReactionProductVector ANNRIGd_DummyModel::DoGenerate() const {
  cout << "ANNRIGd_DummyModel : WARNING! You try to generate reaction "
          "products with the dummy model."
       << endl;
  cout << "ANNRIGd_DummyModel : This is impossible!" << endl;
  cout << "ANNRIGd_DummyModel : No products returned!" << endl;

  ReactionProductVector products;
  return products;
}

} /* namespace ANNRIGdGammaSpecModel */
