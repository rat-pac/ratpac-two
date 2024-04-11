/**
 * @brief  Definition of the GdNCaptureGammaGenerator class.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

#ifndef ANNRIGD_GDNCAPTUREGAMMAGENERATOR_HH_
#define ANNRIGD_GDNCAPTUREGAMMAGENERATOR_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_ModelType.hh"
#include "RAT/ANNRIGd_ReactionProduct.hh"
// STD includes
#include <utility>

//==============================================================================
// FORWARD DECLARATIONS

namespace ANNRIGdGammaSpecModel {
class ANNRIGd_Model;
}

//==============================================================================
// CLASS DEFINITION

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @class   ANNRIGd_GdNCaptureGammaGenerator
 * @brief   Primary generator interface combining all models.
 * @details This class represents the primary interface of the generator. An
 *          instance of this class can hold objects of all the different models.
 *          The class allows to generate reaction products from the deexcitation
 *          of gadolinium after the thermal Gd(n,g) reaction. Products can be
 *          generated from a specific spectral component (continuum, discrete
 *          peaks) of specific gadolinium isotope (156Gd, 158Gd), just from a
 *          specific isotope by properly combining the contributions from the
 *          single spectral components or from "natural gadolinium" as a whole
 *          by combining the contributions of the two isotopes.
 */
class ANNRIGd_GdNCaptureGammaGenerator {
  //------------------------------------------------------------------------------
 public:  // constructors and destructors
  ANNRIGd_GdNCaptureGammaGenerator();
  ANNRIGd_GdNCaptureGammaGenerator(ANNRIGd_Model* mdl156GdCont, ANNRIGd_Model* mdl158GdCont,
                                   ANNRIGd_Model* mdl156GdDisc, ANNRIGd_Model* mdl158GdDisc);
  ANNRIGd_GdNCaptureGammaGenerator(const ANNRIGd_GdNCaptureGammaGenerator& other);
  ~ANNRIGd_GdNCaptureGammaGenerator();

  //------------------------------------------------------------------------------
 private:  // operators
  ANNRIGd_GdNCaptureGammaGenerator& operator=(const ANNRIGd_GdNCaptureGammaGenerator& other);

  //------------------------------------------------------------------------------
 public:  // getters and setters
  bool Set156GdContinuumModel(ANNRIGd_Model* model);
  bool Set156GdDiscreteModel(ANNRIGd_Model* model);

  bool Set158GdContinuumModel(ANNRIGd_Model* model);
  bool Set158GdDiscreteModel(ANNRIGd_Model* model);

  bool SetModel(ANNRIGd_Model* model, ANNRIGd_ModelType::ID modelType);

  //------------------------------------------------------------------------------
 public:  // other methods
  ReactionProductVector Generate_156Gd();
  ReactionProductVector Generate_156Gd_Continuum();
  ReactionProductVector Generate_156Gd_Discrete() const;

  ReactionProductVector Generate_158Gd();
  ReactionProductVector Generate_158Gd_Continuum();
  ReactionProductVector Generate_158Gd_Discrete() const;

  ReactionProductVector Generate_NatGd();

  bool Has156GdContinuumModel() const;
  bool Has156GdDiscreteModel() const;

  bool Has158GdContinuumModel() const;
  bool Has158GdDiscreteModel() const;

  bool HasAllModels() const;

  //------------------------------------------------------------------------------
 private:  // members
  //! @brief   Raw pointer to ANNRIGd_Model instance describing the
  //!          continuum part of the gamma-ray spectrum of 156Gd after
  //!          the thermal 155Gd(n,g) reaction.
  //! @details Must not be NULL.
  ANNRIGd_Model* gd156ContinuumMdl_;

  //! @brief   Raw pointer to ANNRIGd_Model instance describing the
  //!          continuum part of the gamma-ray spectrum of 158Gd after
  //!          the thermal 157Gd(n,g) reaction.
  //! @details Must not be NULL.
  ANNRIGd_Model* gd158ContinuumMdl_;

  //! @brief   Raw pointer to ANNRIGd_Model instance describing the
  //!          discrete peaks of the gamma-ray spectrum of 156Gd after
  //!          the thermal 155Gd(n,g) reaction.
  //! @details Must not be NULL.
  ANNRIGd_Model* gd156DiscreteMdl_;

  //! @brief   Raw pointer to ANNRIGd_Model instance describing the
  //!          discrete peaks of the gamma-ray spectrum of 158Gd after
  //!          the thermal 157Gd(n,g) reaction.
  //! @details Must not be NULL.
  ANNRIGd_Model* gd158DiscreteMdl_;

  //------------------------------------------------------------------------------
 private:  // other methods
  bool CheckModel(const ANNRIGd_Model* model) const;
};

//==============================================================================
// INLINE CLASS METHOD IMPLEMENTATIONS

//______________________________________________________________________________
//! @brief  Tries to set the given model as 156Gd continuum model.
//! @param  model  Raw pointer to model to be set as 156Gd continuum model.
//! @return True, if the model was set successfully. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Set156GdContinuumModel(ANNRIGd_Model* model) {
  return SetModel(model, ANNRIGd_ModelType::Mdl156GdContinuum);
}

//______________________________________________________________________________
//! @brief  Tries to set the given model as 156Gd discrete peak model.
//! @param  model  Raw pointer to model to be set as 156Gd discrete peak model.
//! @return True, if the model was set successfully. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Set156GdDiscreteModel(ANNRIGd_Model* model) {
  return SetModel(model, ANNRIGd_ModelType::Mdl156GdDiscrete);
}

//______________________________________________________________________________
//! @brief  Tries to set the given model as 158Gd continuum model.
//! @param  model  Raw pointer to model to be set as 158Gd continuum model.
//! @return True, if the model was set successfully. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Set158GdContinuumModel(ANNRIGd_Model* model) {
  return SetModel(model, ANNRIGd_ModelType::Mdl158GdContinuum);
}

//______________________________________________________________________________
//! @brief  Tries to set the given model as 158Gd discrete peak model.
//! @param  model  Raw pointer to model to be set as 158Gd discrete peak model.
//! @return True, if the model was set successfully. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Set158GdDiscreteModel(ANNRIGd_Model* model) {
  return SetModel(model, ANNRIGd_ModelType::Mdl158GdDiscrete);
}

//______________________________________________________________________________
//! @brief   Checks if a model for the continuum spectrum part of 156Gd is set.
//! @details It is checked if:
//!          - the raw pointer to the model is not NULL,
//!          - the model does not have the 'MdlDummy' model type ID.
//! @return  True, if all the checks return true. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Has156GdContinuumModel() const { return CheckModel(gd156ContinuumMdl_); }

//______________________________________________________________________________
//! @brief   Checks if a model for the discrete peaks of 156Gd is set.
//! @details It is checked if:
//!          - the raw pointer to the model is not NULL,
//!          - the model does not have the 'MdlDummy' model type ID.
//! @return  True, if all the checks return true. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Has156GdDiscreteModel() const { return CheckModel(gd156DiscreteMdl_); }

//______________________________________________________________________________
//! @brief   Checks if a model for the continuum spectrum part of 158Gd is set.
//! @details It is checked if:
//!          - the raw pointer to the model is not NULL,
//!          - the model does not have the 'MdlDummy' model type ID.
//! @return  True, if all the checks return true. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Has158GdContinuumModel() const { return CheckModel(gd158ContinuumMdl_); }

//______________________________________________________________________________
//! @brief   Checks if a model for the discrete peaks of 158Gd is set.
//! @details It is checked if:
//!          - the raw pointer to the model is not NULL,
//!          - the model does not have the 'MdlDummy' model type ID.
//! @return  True, if all the checks return true. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::Has158GdDiscreteModel() const { return CheckModel(gd158DiscreteMdl_); }

//______________________________________________________________________________
//! @brief   Checks if all models are set.
//! @details It is checked if:
//!          - a model for the continuum spectrum part of 156Gd and
//!          - a model for the continuum spectrum part of 158Gd and
//!          - a model for the discrete peaks of 156Gd and
//!          - a model for the discrete peaks of 158Gd and
//!          is set.
//! @return  True, if all the checks return true. Otherwise false.
inline bool ANNRIGd_GdNCaptureGammaGenerator::HasAllModels() const {
  return Has156GdContinuumModel() and Has158GdContinuumModel() and Has156GdDiscreteModel() and Has158GdDiscreteModel();
}

}  // namespace ANNRIGdGammaSpecModel

#endif /* ANNRIGD_GDNCAPTUREGAMMAGENERATOR_HH_ */
