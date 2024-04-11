/**
 * @brief  Implementations for the ANNRIGd_Model base class.
 * @author Sebastian Lorenz
 * @date   2017-08-03
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_Model.hh"
// STD includes
#include <iostream>
using std::cout;
using std::endl;

//==============================================================================
// BASE CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructor with parameter.
 * @details If the given string for the model name is empty, the string
 *          '-UNKNOWN-' is set as name.
 * @param   name  Name of the model.
 * @param   id  Type ID of the model.
 */
ANNRIGd_Model::ANNRIGd_Model(const std::string& name, ANNRIGd_ModelType::ID id) : name_(name), id_(id) {
  if (name_.empty()) {
    cout << "ANNRIGd_Model : WARNING! Given model name string is empty. "
            "Setting '-UNKNOWN-' as name."
         << endl;
    name_ = "-UNKNOWN-";
  }
}

//______________________________________________________________________________
//! @brief Destructor.
ANNRIGd_Model::~ANNRIGd_Model() { /* Nothing done here. */
}

} /* namespace ANNRIGdGammaSpecModel */
