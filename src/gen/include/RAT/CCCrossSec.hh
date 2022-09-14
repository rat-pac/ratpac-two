////////////////////////////////////////////////////////////////////
///
/// \class RAT::CCCrossSec
///
/// \brief Calculates neutrino-nucleus charged current interaction on Lithium-7.
/// (based on ES generator).
///
/// \author Max Smiley <masmiley@berkeley.edu> -- contact person
///
/// REVISION HISTORY:\n
///
///
/// \details Calculates the neutrino-nucleus CC cross section.
/// 		Some remarks concerning the calculations.
///			\param E,Enu 	: Neutrino energy (MeV).
///			\param T,Te		: Recoil electron energy (MeV).
///			\return Cross section in units of $10^{-42}cm^{2}$
///
///
////////////////////////////////////////////////////////////////////

#ifndef __RAT_CCCrossSec__
#define __RAT_CCCrossSec__

// G4 headers
#include <globals.hh>
// RAT headers
#include <RAT/GLG4StringUtil.hh>
#include <RAT/LinearInterp.hh>
// ROOT headers
#include <TGraph.h>

#include <vector>

namespace RAT {

// Forward declarations within the namespace
class CCCrossSecMessenger;

class CCCrossSec {
 public:
  enum NuEType { nue, nuebar, numu, numubar };

  CCCrossSec(const char *flavor = "nue");

  ~CCCrossSec();

  // Set's the defaults for the calculation
  void Defaults();

  /**
   * @brief Calculate the total cross section for the neutrino energy Enu.
   *
   * @param Enu
   * @return total cross section in units of \f$ 10^{-42} cm^{2} \f$ .
   */
  double Sigma(const double Enu) const;

  /**
   * @brief Return a vector with the allowed electron KE for each level for an
   * incoming neutrino with energy Enu.
   * @param Enu Incoming neutrino energy (MeV)
   * @return Vector with the allowed electron KE in MeV .
   */
  std::vector<double> CalcAllowedElectronKE(const double Enu) const;

  /**
   * @brief Return a vector with the nuclear excitation for each level for an
   * incoming neutrino with energy Enu.
   * @param Enu Incoming neutrino energy (MeV)
   * @return Vector with the allowed nuclear excitation in MeV .
   */
  std::vector<double> CalcAllowedNuclearEx(const double Enu) const;

  /**
   * @brief Return a vector with the allowed transition types for each level for
   * an incoming neutrino with energy Enu.
   * @param Enu Incoming neutrino energy (MeV)
   * @return Vector with the allowed electron transition types .
   */
  std::vector<double> GetAllowedTransitionTypes(const double Enu) const;

  /**
   * @brief Return a vector with the scaled differential cross section
   * normalizations for each level for an incoming neutrino with energy Enu.
   * @param Enu Incoming neutrino energy (MeV)
   * @return Vector with the (relative) level normalizations for \f$
   * \frac{d\sigma}{dT} \f$ in arbitrary units.
   */
  std::vector<double> CalcdSigmadTNorms(const double Enu) const;

  /**
   * Returns the global normalization of the cross section calculation.
   * For precision reasons, the cross-section is performed on a different scale,
   * and therefore any result returned by the calculation is missing this scale,
   * which has to be applied separately.
   *
   * @return cross section scaling factor (1e-42)
   */
  double CrossSecNorm() const { return 1e-42; };

 private:
  NuEType fReaction;         /// Reaction type
  std::string fReactionStr;  /// String characterizing the reaction type

  // Some constants
  static const double fGf;      /// Fermi constant (GeV^-2)
  static const double fhbarc;   /// hbar*c (MeV*fm)
  static const double fhbarc2;  /// hbar*c^2(GeV^2 mb)
  static const double falpha;   /// radiative correction term

  /**
   * This variable is defined as static (and not const) because it can be
   * changed in the macro file. However, the change should propagate to all
   * instances of the class
   */
  static double fsinthetaW2;

  double fMe;  /// electron mass

  std::vector<double> fLevels;      // Energy of level transitions (mass change + excitation energy)
  std::vector<double> fLevelTypes;  // Type of transition (F = 0, GT = 1)
  std::vector<double> fNorms;       // E_e indepedent normalization factors for each transition

  CCCrossSecMessenger *fMessenger;
};

}  // namespace RAT

#endif
