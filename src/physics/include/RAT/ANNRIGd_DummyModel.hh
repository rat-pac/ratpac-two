/**
 * @brief  Definition of the ANNRIGd_DummyModel class used in the
 * 		   ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

#ifndef ANNRIGD_DUMMYMODEL_HH_
#define ANNRIGD_DUMMYMODEL_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_Model.hh"

//==============================================================================
// CLASS DEFINITION

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @class   ANNRIGd_DummyModel
 * @brief   Place holder that represents / is to be used in the absence of a
 *          real model.
 */
class ANNRIGd_DummyModel : public ANNRIGdGammaSpecModel::ANNRIGd_Model {
  //------------------------------------------------------------------------------
 public:  // constructors and destructors
  ANNRIGd_DummyModel();
  ~ANNRIGd_DummyModel();

  //------------------------------------------------------------------------------
 public:  // other methods
  ANNRIGd_DummyModel* Clone() const;

  //------------------------------------------------------------------------------
 private:  // other methods
  ANNRIGd_DummyModel* DoClone() const;
  ReactionProductVector DoGenerate() const;
};

//==============================================================================
// INLINE CLASS METHOD IMPLEMENTATIONS

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::Clone() const
inline ANNRIGd_DummyModel* ANNRIGd_DummyModel::Clone() const { return DoClone(); }

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::DpClone() const
inline ANNRIGd_DummyModel* ANNRIGd_DummyModel::DoClone() const { return new ANNRIGd_DummyModel(*this); }

} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_DUMMYMODEL_HH_ */
