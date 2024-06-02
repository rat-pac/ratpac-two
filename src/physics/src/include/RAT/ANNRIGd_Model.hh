/**
 * @brief  Definition of the ANNRIGd_Model base class used in the ANNRI-Gd
 *         generator code.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

#ifndef ANNRIGD_MODEL_HH_
#define ANNRIGD_MODEL_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ModelType.hh"
#include "RAT/ANNRIGd_ReactionProduct.hh"
// STD includes
#include <string>

//==============================================================================
// BASE CLASS DEFINTION

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @class   ANNRIGd_Model
 * @brief   Base class for a model component to be used by the ANNRI-Gd gamma
 *          ray generator to simulate the gamma-ray spectrum of gadolinium
 *          after thermal neutron capture.
 * @details This class uses a non-virtual public interface with the Generate()
 *          method. The actual gamma generation process must be defined in
 *          a derived class my implementing the pure virtual, private method
 *          DoGenerate().
 */
class ANNRIGd_Model {
  //------------------------------------------------------------------------------
 public:  // constructors and destructors
  ANNRIGd_Model(const std::string& name, ANNRIGd_ModelType::ID id);
  virtual ~ANNRIGd_Model();

  //------------------------------------------------------------------------------
 public:  // getters and setters
  ANNRIGd_ModelType::ID GetModelTypeID() const;
  const std::string& GetName() const;

  //------------------------------------------------------------------------------
 public:  // other methods
  ANNRIGd_Model* Clone() const;
  ReactionProductVector Generate() const;
  bool IsDummyModel() const;
  bool IsKnownModel() const;

  //------------------------------------------------------------------------------
 private:  // other methods
  //! @brief   Clones this object.
  //! @details Must be implemented in a derived class.
  //! @post    Returned raw pointer is not NULL.
  //! @return  Raw pointer to a clone of this object.
  virtual ANNRIGd_Model* DoClone() const = 0;

  //! @brief   Generates the reaction products (gamma-rays, IC electrons)
  //!          from the thermal neutron capture on gadolinium.
  //! @details Must be implemented in a derived class.
  //! @return  ReactionProductVector instance containing the reaction products.
  virtual ReactionProductVector DoGenerate() const = 0;

  //------------------------------------------------------------------------------
 private:                     // member variables
  std::string name_;          //!< Name of the model. Must not be empty.
  ANNRIGd_ModelType::ID id_;  //!< Model type ID.
};

//==============================================================================
// INLINE BASE CLASS METHOD IMPLEMENTATIONS

//______________________________________________________________________________
//! @brief  Returns type ID of the model.
//! @return Type ID of the model.
inline ANNRIGd_ModelType::ID ANNRIGd_Model::GetModelTypeID() const { return id_; }

//______________________________________________________________________________
//! @brief  Returns name of the model.
//! @post   Returned string is not empty.
//! @return Name of model.
inline const std::string& ANNRIGd_Model::GetName() const { return name_; }

//______________________________________________________________________________
//! @brief   Clones this object.
//! @details Forwards call to the pure virtual method DoClone() that must be
//!          implemented by a derived class.
//! @post    Returned raw pointer is not NULL.
//! @return  Raw pointer to a clone of this object.
inline ANNRIGd_Model* ANNRIGd_Model::Clone() const { return DoClone(); }

//______________________________________________________________________________
//! @brief   Generates the products of the thermal Gd(n,g) reaction.
//! @details Forwards method call to the pure virtual method DoGenerate() that
//!          must be implemented by a derived class.
//! @return  ReactionProductVector containing the reaction products.
inline ReactionProductVector ANNRIGd_Model::Generate() const { return DoGenerate(); }

//______________________________________________________________________________
//! @brief  Checks if the model is a dummy model.
//! @return True, if the model type ID is the model type ID of the dummy model.
//!         Otherwise false.
inline bool ANNRIGd_Model::IsDummyModel() const { return id_ == ANNRIGd_ModelType::MdlDummy; }

//______________________________________________________________________________
//! @brief  Checks if the model is a known model.
//! @return True, if the model's type ID is known. Otherwise false.
inline bool ANNRIGd_Model::IsKnownModel() const { return ANNRIGd_ModelType::IsKnown(id_); }

} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_MODEL_HH_ */
