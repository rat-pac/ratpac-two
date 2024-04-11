/**
 * @brief  Definition of the ANNRIGd_156GdContinuumModelV2 class used in the
 * 		   ANNRI-Gd generator code.
 * @author Sebastian Lorenz
 * @date   2017-11-06
 */

#ifndef ANNRIGD_156GDCONTINUUMMODELV2_HH_
#define ANNRIGD_156GDCONTINUUMMODELV2_HH_

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_Auxiliary.hh"
#include "RAT/ANNRIGd_Model.hh"
// STD includes
#include <string>

//==============================================================================
// FORWARD DECLARATIONS

// ROOT fwd declarations
class TH2D;

//==============================================================================
// CLASS DEFINITION

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @class   ANNRIGd_156GdContinuumModelV2
 * @brief   Class describing a model for the continuum part of the gamma-ray
 *          spectrum of 156Gd* after the thermal 155Gd(n,g) reaction.
 * @details Gamma-rays from the thermal 155Gd(n,g) reaction are generated with
 *          the help of one, precomputed look-up table. The look-ups of
 *          gamma-ray energies are done based on a random number from ]0,1[.
 */
class ANNRIGd_156GdContinuumModelV2 : public ANNRIGd_Model {
  //------------------------------------------------------------------------------
 public:  // constructors and destructors
  ANNRIGd_156GdContinuumModelV2(const std::string& inDataFileName);
  ANNRIGd_156GdContinuumModelV2(const ANNRIGd_156GdContinuumModelV2& other);
  ~ANNRIGd_156GdContinuumModelV2();

  //------------------------------------------------------------------------------
 public:  // operators
  ANNRIGd_156GdContinuumModelV2& operator=(const ANNRIGd_156GdContinuumModelV2& other);

  //------------------------------------------------------------------------------
 public:  // other methods
  ANNRIGd_156GdContinuumModelV2* Clone() const;

  //------------------------------------------------------------------------------
 private:  // other methods
  ANNRIGd_156GdContinuumModelV2* DoClone() const;
  ReactionProductVector DoGenerate() const;
  double GetGammaEnergy(double eRes) const;

  void Initialize(const std::string& inDataFileName);

  //------------------------------------------------------------------------------
 private:                       // member variables
  static int sInstanceCounter;  //!< counts number of created instances

  double eMax_;  //!< maximum available energy [MeV]
  double dE_;    //!< energy step in LUT [MeV]
  int instID_;   //!< id of the instance of this class

  //! @brief   Look-table for transitions contributing to the spectral continuum
  //!          part.
  //! @details X-Axis  : Excitation energy [MeV].
  //!          Y-Axis  : Bins covering [0,1[ to access the table information
  //!                    by a random number in ]0,1[.
  //!          Content : Gamma-ray energy [MeV].
  TH2D* lut_;
};

//==============================================================================
// INLINE CLASS METHOD IMPLEMENTATIONS

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::Clone() const
inline ANNRIGd_156GdContinuumModelV2* ANNRIGd_156GdContinuumModelV2::Clone() const { return DoClone(); }

//______________________________________________________________________________
//! @copydoc ANNRIGd_Model::DoClone() const
inline ANNRIGd_156GdContinuumModelV2* ANNRIGd_156GdContinuumModelV2::DoClone() const {
  return new ANNRIGd_156GdContinuumModelV2(*this);
}

} /* namespace ANNRIGdGammaSpecModel */
#endif /* ANNRIGD_156GDCONTINUUMMODELV2_HH_ */
