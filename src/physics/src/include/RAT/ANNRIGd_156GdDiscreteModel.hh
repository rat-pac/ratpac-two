/**
 * @brief  Definition of the ANNRIGd_156GdDiscreteModel class used in the
 * 		   ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-08-06
 */

#ifndef ANNRIGD_156GDDISCRETEMODEL_HH_
#define ANNRIGD_156GDDISCRETEMODEL_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_Model.hh"

//==============================================================================
// CLASS DEFINITION

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @class   ANNRIGd_156GdDiscreteModel
 * @brief   Class describing a model for the discrete peaks in the gamma-ray
 *          spectrum of 156Gd* after the thermal 155Gd(n,g) reaction.
 * @details In the model, the gamma-rays forming the discrete peaks originate
 *          from fixed transitions steps. Therefore, the sequence of emitted
 *          gamma-ray energies is hard coded.
 */
class ANNRIGd_156GdDiscreteModel : public ANNRIGdGammaSpecModel::ANNRIGd_Model {
  //------------------------------------------------------------------------------
 public:  // constructors and destructors
  ANNRIGd_156GdDiscreteModel();
  ~ANNRIGd_156GdDiscreteModel();

  //------------------------------------------------------------------------------
 public:  // other methods
  ANNRIGd_156GdDiscreteModel* Clone() const;

  //------------------------------------------------------------------------------
 private:  // other methods
  ANNRIGd_156GdDiscreteModel* DoClone() const;
  ReactionProductVector DoGenerate() const;

  void Fill_156Gd_Discrete01(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete02(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete03(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete04(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete05(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete06(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete07(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete08(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete09(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete10(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete11(ReactionProductVector& products) const;
  void Fill_156Gd_Discrete12(ReactionProductVector& products) const;
};

//==============================================================================
// INLINE CLASS METHOD IMPLEMENTATIONS

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::Clone() const
inline ANNRIGd_156GdDiscreteModel* ANNRIGd_156GdDiscreteModel::Clone() const { return DoClone(); }

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::DpClone() const
inline ANNRIGd_156GdDiscreteModel* ANNRIGd_156GdDiscreteModel::DoClone() const {
  return new ANNRIGd_156GdDiscreteModel(*this);
}

} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_156GDDISCRETEMODEL_HH_ */
