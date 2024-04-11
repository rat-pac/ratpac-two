/**
 * @brief  Implementations for the GdNCaptureGammaGenerator class.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

//==============================================================================
// INCLUDES

// ANNRIGdSpectrumModel namespace includes
#include "RAT/ANNRIGd_GdNCaptureGammaGenerator.hh"

#include "RAT/ANNRIGd_DummyModel.hh"
#include "RAT/ANNRIGd_Random.hh"
// STD includes
#include <iostream>

namespace Rnd = ANNRIGdGammaSpecModel::Random;
using std::cerr;
using std::cout;
using std::endl;

//==============================================================================
// CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructor.
 * @details All raw pointers to models are set to point to separate
 *          ANNRIGd_DummyModel instances.
 */
ANNRIGd_GdNCaptureGammaGenerator::ANNRIGd_GdNCaptureGammaGenerator()
    : gd156ContinuumMdl_(new ANNRIGd_DummyModel()),
      gd158ContinuumMdl_(new ANNRIGd_DummyModel()),
      gd156DiscreteMdl_(new ANNRIGd_DummyModel()),
      gd158DiscreteMdl_(new ANNRIGd_DummyModel()) {
  cout << "ANNRIGd_GdNCaptureGammaGenerator : Created with default setup." << endl;
  cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Only dummy models set." << endl;
}

//______________________________________________________________________________
/**
 * @brief   Copy-constructor.
 * @details A deep copy is made by cloning all objects referred to by
 *          raw pointers.
 * @param   other  GdNCaptureGammaGenerator instance that shall be copied.
 */
ANNRIGd_GdNCaptureGammaGenerator::ANNRIGd_GdNCaptureGammaGenerator(const ANNRIGd_GdNCaptureGammaGenerator& other)
    : gd156ContinuumMdl_(other.gd156ContinuumMdl_->Clone()),
      gd158ContinuumMdl_(other.gd158ContinuumMdl_->Clone()),
      gd156DiscreteMdl_(other.gd156DiscreteMdl_->Clone()),
      gd158DiscreteMdl_(other.gd158DiscreteMdl_->Clone()) { /* Nothing is done here! */
}

//______________________________________________________________________________
/**
 * @brief Constructor with parameters.
 * @param mdl156GdCont  Raw pointer referring to the continuum model of 156Gd
 *        that shall be set.
 * @param mdl158GdCont  Raw pointer referring to the continuum model of 158Gd
 *        that shall be set.
 * @param mdl156GdDisc  Raw pointer referring to the discrete peak model of
 *        156Gd that shall be set.
 * @param mdl158GdDisc  Raw pointer referring to the discrete peak model of
 *        158Gd that shall be set.
 */
ANNRIGd_GdNCaptureGammaGenerator::ANNRIGd_GdNCaptureGammaGenerator(ANNRIGd_Model* mdl156GdCont,
                                                                   ANNRIGd_Model* mdl158GdCont,
                                                                   ANNRIGd_Model* mdl156GdDisc,
                                                                   ANNRIGd_Model* mdl158GdDisc)
    : gd156ContinuumMdl_(new ANNRIGd_DummyModel()),
      gd158ContinuumMdl_(new ANNRIGd_DummyModel()),
      gd156DiscreteMdl_(new ANNRIGd_DummyModel()),
      gd158DiscreteMdl_(new ANNRIGd_DummyModel()) {
  if (Set156GdContinuumModel(mdl156GdCont) and Set158GdContinuumModel(mdl158GdCont) and
      Set156GdDiscreteModel(mdl156GdDisc) and Set158GdDiscreteModel(mdl158GdDisc)) {
    cout << "GdNCaptureGammaGenerator : Created with all models "
            "successfully set."
         << endl;
  } else {
    cout << "GdNCaptureGammaGenerator : Created but one or more models "
            "could not be set."
         << endl;
  }
}

//______________________________________________________________________________
//! @brief Destructor.
ANNRIGd_GdNCaptureGammaGenerator::~ANNRIGd_GdNCaptureGammaGenerator() {
  delete gd156ContinuumMdl_;
  delete gd158ContinuumMdl_;
  delete gd156DiscreteMdl_;
  delete gd158DiscreteMdl_;

  gd156ContinuumMdl_ = 0;
  gd158ContinuumMdl_ = 0;
  gd156DiscreteMdl_ = 0;
  gd158DiscreteMdl_ = 0;
}

//______________________________________________________________________________
/**
 * @brief   Copy-assignment operator.
 * @details Uses copy-and-swap. The old member objects referred to by raw pointers
 *          are deleted.
 * @param   other  GdNCaptureGammaGenerator instance to assign by copy.
 * @return  Reference to this instance.
 */
ANNRIGd_GdNCaptureGammaGenerator& ANNRIGd_GdNCaptureGammaGenerator::operator=(
    const ANNRIGd_GdNCaptureGammaGenerator& other) {
  ANNRIGd_Model* gd156ContinuumMdl = other.gd156ContinuumMdl_->Clone();
  ANNRIGd_Model* gd158ContinuumMdl = other.gd158ContinuumMdl_->Clone();
  ANNRIGd_Model* gd156DiscreteMdl = other.gd156DiscreteMdl_->Clone();
  ANNRIGd_Model* gd158DiscreteMdl = other.gd158DiscreteMdl_->Clone();

  delete gd156ContinuumMdl_;
  gd156ContinuumMdl_ = gd156ContinuumMdl;
  gd156ContinuumMdl = 0;

  delete gd158ContinuumMdl_;
  gd158ContinuumMdl_ = gd158ContinuumMdl;
  gd158ContinuumMdl = 0;

  delete gd156DiscreteMdl_;
  gd156DiscreteMdl_ = gd156DiscreteMdl;
  gd156DiscreteMdl = 0;

  delete gd158DiscreteMdl_;
  gd158DiscreteMdl_ = gd158DiscreteMdl;
  gd158DiscreteMdl = 0;

  return *this;
}

//______________________________________________________________________________
/**
 * @brief   Sets given model as given model type if there are no problems.
 * @details It is checked if
 *          - there is a model to set (given raw pointer not NULL),
 *          - the given model type ID is known,
 *          - the given model type ID is not the dummy model ID,
 *          - the model type ID of the given model is known,
 *          - the model type ID of the given model is not the dummy model ID,
 *          - the model type ID of the given model and the given model type ID
 *            match.
 *          If all checks passed successfully, the new model is set and the old
 *          one is deleted.
 * @param   model  Raw pointer referring to the new model to set. The
 *          GdNCaptureGammaGenerator instance takes over lifetime management of
 *          the underlying object.
 * @param   modelType  ID of model type the given model shall be set for.
 * @return  True, if the given model was set. Otherwise false.
 */
bool ANNRIGd_GdNCaptureGammaGenerator::SetModel(ANNRIGd_Model* model, ANNRIGd_ModelType::ID modelType) {
  bool modelSet = false;  // flag indicating if the new model was set.

  if (not model) {  // is a model to set?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! There is no model to set." << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  } else if (not ANNRIGd_ModelType::IsKnown(modelType)) {  // given model type ID known?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Given model ID is unknown." << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  } else if (modelType == ANNRIGd_ModelType::MdlDummy) {  // given model type ID not dummy model ID?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Cannot set <" << model->GetName() << "> as dummy model."
         << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  } else if (not model->IsKnownModel()) {  // model type known?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Given model <" << model->GetName()
         << "> is of unknown model type." << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  } else if (model->IsDummyModel()) {  // given model type not dummy?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Given model <" << model->GetName() << "> is a dummy model."
         << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  }
  if (model->GetModelTypeID() not_eq modelType) {  // model type IDs match?
    cout << "ANNRIGd_GdNCaptureGammaGenerator : WARNING! Cannot set model <" << model->GetName()
         << ">, which is classified as <" << ANNRIGd_ModelType::ToString(model->GetModelTypeID()) << ">, as a <"
         << ANNRIGd_ModelType::ToString(modelType) << ">." << endl;
    cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
  } else {
    switch (modelType) {
      case ANNRIGd_ModelType::Mdl156GdContinuum:
        delete gd156ContinuumMdl_;
        gd156ContinuumMdl_ = model;
        modelSet = true;
        break;
      case ANNRIGd_ModelType::Mdl158GdContinuum:
        delete gd158ContinuumMdl_;
        gd158ContinuumMdl_ = model;
        modelSet = true;
        break;
      case ANNRIGd_ModelType::Mdl156GdDiscrete:
        delete gd156DiscreteMdl_;
        gd156DiscreteMdl_ = model;
        modelSet = true;
        break;
      case ANNRIGd_ModelType::Mdl158GdDiscrete:
        delete gd158DiscreteMdl_;
        gd158DiscreteMdl_ = model;
        modelSet = true;
        break;
      default:
        break;
    }

    if (modelSet) {
      cout << "ANNRIGd_GdNCaptureGammaGenerator : Successfully set <" << model->GetName() << "> as <"
           << ANNRIGd_ModelType::ToString(modelType) << ">." << endl;
    } else {
      cout << "ANNRIGd_GdNCaptureGammaGenerator : Uuuups! Something went "
              "wrong! You should not get here..."
           << endl;
      cout << "ANNRIGd_GdNCaptureGammaGenerator : Method call ignored!" << endl;
    }
  }

  return modelSet;
}

//______________________________________________________________________________
/**
 * @brief   Randomly generates the product particles from 155Gd(n,g) or
 *          157Gd(n,g) in natural gadolinium.
 * @details The abundance-weighted thermal n-capture cross-sections of the
 *          reactions are:
 *          - 155Gd(n,g) : 0.1480 *  60900 b =  9013.2 b
 *          - 157Gd(n,g) : 0.1565 * 254000 b = 39751.0 b
 *          - Total      :                   = 48761.2 b
 *          The fraction 157Gd(n,g) / Total is about 0.81517. The actual neutron
 *          target is chosen by comparing a random number from ]0,1[ to this
 *          ratio.
 *
 *          Sources:
 *          - Abundances of nat. gadolinum:
 *            [https://en.wikipedia.org/wiki/Gadolinium] (accessed 2017-07-25).
 *          - Thermal neutron capture cross-sections:
 *            [Nuclear Data Sheets 112 (2011) 2887-2996]
 * @return ReactionProductVector filled with information on the reaction
 *         products from either 155Gd(n,g) or 157Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_NatGd() {
  const double isoSelection = Rnd::Uniform();
  if (isoSelection < 0.81517)
    return Generate_158Gd();  // 158Gd deexcitation from n-capture on 157Gd
  else
    return Generate_156Gd();  // 156Gd deexcitation from n-capture on 155Gd
}

//______________________________________________________________________________
/**
 * @brief   Randomly generates the product particles from the continuum part or
 *          the discrete transitions of 156Gd after thermal 155Gd(n,g).
 * @details The continuum contribution from the deexcitation of 156Gd is 97.22%.
 *          The remaining 2.78% are the contribution from the discrete transitions.
 *
 *          Sources:
 *          - Fractions of continuum and discrete peaks:
 *            [Our data] (TODO REFERENCE TO PUBLICATION)
 * @return  ReactionProductVector filled with information on the reaction
 *          products from either the continuum or the discrete peaks from the
 *          deexcitation or 156Gd after thermal 155Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_156Gd() {
  const double modeSelection = Rnd::Uniform();
  if (modeSelection < 0.9722)
    return Generate_156Gd_Continuum();
  else
    return Generate_156Gd_Discrete();
}

//______________________________________________________________________________
/**
 * @brief  Randomly generates product particles from the continuum spectrum part
 *         of 156Gd after thermal 155Gd(n,g).
 * @return ReactionProductVector filled with information on the reaction
 *         products from the continuum spectrum following the deexcitation of
 *         156Gd after thermal 155Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_156Gd_Continuum() {
  return gd156ContinuumMdl_->Generate();
}

//______________________________________________________________________________
/**
 * @brief  Randomly generates product particles from the discrete peaks
 *         of 156Gd after thermal 155Gd(n,g).
 * @return ReactionProductVector filled with information on the reaction
 *         products from the discrete peaks following the deexcitation of
 *         156Gd after thermal 155Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_156Gd_Discrete() const {
  return gd156DiscreteMdl_->Generate();
}

//______________________________________________________________________________
/**
 * @brief   Randomly generates the product particles from the continuum part or
 *          the discrete transitions of 158Gd after thermal 157Gd(n,g).
 * @details The continuum contribution from the deexcitation of 156Gd is 93.062%.
 *          The remaining 6.938% are the contribution from the discrete transitions.
 *
 *          Sources:
 *          - Fractions of continuum and discrete peaks:
 *            [Our data] (TODO REFERENCE TO PUBLICATION)
 * @return  ReactionProductVector filled with information on the reaction
 *          products from either the continuum or the discrete peaks from the
 *          deexcitation of 1586Gd after thermal 157Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_158Gd() {
  const double modeSelection = G4UniformRand();
  if (modeSelection < 0.93062)
    return Generate_158Gd_Continuum();
  else
    return Generate_158Gd_Discrete();
}

//______________________________________________________________________________
/**
 * @brief  Randomly generates product particles from the continuum spectrum part
 *         of 158Gd after thermal 157Gd(n,g).
 * @return ReactionProductVector filled with information on the reaction
 *         products from the continuum spectrum following the deexcitation of
 *         158Gd after thermal 157Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_158Gd_Continuum() {
  return gd158ContinuumMdl_->Generate();
}

//______________________________________________________________________________
/**
 * @brief  Randomly generates product particles from the discrete peaks
 *         of 158Gd after thermal 157Gd(n,g).
 * @return ReactionProductVector filled with information on the reaction
 *         products from the discrete peaks following the deexcitation of
 *         158Gd after thermal 157Gd(n,g).
 */
ReactionProductVector ANNRIGd_GdNCaptureGammaGenerator::Generate_158Gd_Discrete() const {
  return gd158DiscreteMdl_->Generate();
}

//------------------------------------------------------------------------------
// PRIVATE METHODS

//______________________________________________________________________________
/**
 * @brief   Checks if the given raw pointer points to a valid model.
 * @details It is checked if:
 *          - the raw pointer to the model is not NULL,
 *          - the model is not a dummy model.
 * @return  True, if all the checks return true. Otherwise false.
 */
bool ANNRIGd_GdNCaptureGammaGenerator::CheckModel(const ANNRIGd_Model* model) const {
  return model and model->IsDummyModel();
}

}  // namespace ANNRIGdGammaSpecModel
