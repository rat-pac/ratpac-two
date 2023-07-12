#ifndef __RAT_CCgen__
#define __RAT_CCgen__

////////////////////////////////////////////////////////////////////
///
/// \class RAT::CCgen
/// \author Max Smiley <masmiley@berkeley.edu> -- contact person
/// \date 26-Aug-2019
///
/// \brief Implements the generation of a neutrino-nucleus charged current
/// interaction.
///
/// This class is the workhorse of the generator, separating the Geant4 specific
/// methods from a more physics oriented structure. It is based on the original
/// elastic scattering implementation by Joe Formaggio and the following
/// adaptation by Bill Seligman. However the whole class was re-written later to
/// adapt it for the final use of charged current interactions.
///
////////////////////////////////////////////////////////////////////

#include <CLHEP/Random/RandGeneral.h>
#include <CLHEP/Vector/LorentzVector.h>
#include <TF1.h>

#include <G4LorentzVector.hh>
#include <G4ThreeVector.hh>
#include <RAT/LinearInterp.hh>

/// Forward declarations.
class TGraph;
class RandGeneral;

namespace RAT {

/// Forward declarations within the namespace
class CCCrossSec;

class CCgen {
 public:
  CCgen();
  ~CCgen();

  // Generate random event vectors
  //    Pass in the neutrino direction (unit vector)
  //    Returns 4-momentum vectors for resulting electron.
  /**
   * Generate random event vectors.
   *
   * Pass in the neutrino direction (unit vector).
   * \param[in] nu_dir Incoming neutrino direction (lab coordinates).
   * \param[out] neutrino Outgoing neutrino direction (lab coordinates. Not
   * used). \param[out] electron Outgoing electron direction (lab coordinates).
   * \return 4-momentum vectors for resulting electron.
   */
  void GenerateEvent(const G4ThreeVector &nu_dir, G4LorentzVector &neutrino, G4LorentzVector &electron,
                     double &e_nucleus);

  /**
   * Setter for the flux to use.
   *
   * \param[in] nutype Key to the database to load the flux.
   */
  void SetNuType(const G4String &nutype);

  /** Getter for the spectrum being used */
  inline const G4String &GetNuType() const { return fNuFlavor; };

  /**
   * Setter for the neutrino flavor being generated.
   *
   * This parameter is passed down into the cross-section to calculate the
   * correct shape.
   *
   * @param nuflavor Flavor of the neutrino being calculated. Can be one of
   * (<tt>nue,numu,nuebar,numubar</tt>). \attention \f$ \sigma_{\mu} =
   * \sigma_{\tau}\f$
   *
   */
  void SetNuFlavor(const G4String &nuflavor);
  /** Getter for neutrino flavor */
  inline const G4String &GetNuFlavor() const { return fNuFlavor; };

  /**
   * @brief Getter for the total neutrino flux.
   * @return total neutrino flux in \f$ s^{-1} cm^{-2} \f$
   */
  inline G4double GetTotalFlux() {
    // Return the neutrino flux as the value loaded from the database
    return fTotalFlux;
  }

  /**
   * @brief Getter for the SSM event rate per target for this flux.
   *
   * The SSM event rate is obtained from the product of the cross section and
   * the flux: \f$ R_{\nu} = \sigma \times \Phi_{\nu} \f$
   *
   * @return The event Rate predicted by the SSM (in Hz)
   */
  G4double GetRatePerTarget();

  /**
   * Getter of the DB entry to input the spectrum from.
   * @return name of the DB name.
   */
  const G4String GetDBName() const { return fDBName; }

  /**
   * Setter of the DB name. Defaults to \'SOLAR\'
   * @param[in] name of the database entry to look at.
   */
  void SetDBName(const G4String name);

 private:
  /** Private member for load the database and cross-section data. */
  void LoadGenerator();

  /** Generate the interaction given the neutrino energy and the recoil angle.*/
  void GenInteraction(double &Enu, double &CosThetaLab);

  /** Resets internal vectors. To be removed. */
  void Reset();
  /** Show internal state of calculations. To be removed.*/
  void Show();

  /** Sampler of neutrino energy from the spectrum. */
  G4double SampleNuEnergy();

  /** Sampler of recoil electron energy from the differential cross section. */
  G4double SampleRecoilEnergy(G4double Enu, int &Transition, double &Enucleus);

  /** Sampler of recoil electron angle from the differential cross section. */
  G4double SampleRecoilAngle(G4double Enu, G4double Te, int Transition);

 protected:
  /** Private method to check if generator is loaded. */
  inline G4bool GetGenLoaded() { return fGenLoaded; };

  /** Generator type */
  G4String fGenType;
  /** Neutrino type */
  G4String fNuType;

  /** Neutrino flavor */
  G4String fNuFlavor;

  /** Instance of cross-section class */
  CCCrossSec *fXS;

  /**
   *  @brief Spectrum shape to be sampled
   *
   *  Using ROOT TGraph to make use of it's nice evaluator.
   */
  TGraph *fNuSpectrum;

  TF1 *fFermiAngle;

  TF1 *fGTAngle;

  /** vector of neutrino energy points in the neutrino spectrum shape. */
  std::vector<double> fEnuTbl;

  /** Normalized flux in the neutrino spectrum shape. */
  std::vector<double> fFluxTbl;

  /** Recoil upper limit for the electron.*/
  G4double fEmax;
  /** Recoil lower limit for the electron.*/
  G4double fEmin;

  /** Recoil upper limit for the electron.*/
  G4double fEnuMax;
  /** Recoil lower limit for the electron.*/
  G4double fEnuMin;

  /** Maximum flux in spectrum shape */
  G4double fFluxMax;

  /** Generator loaded flag. */
  G4bool fGenLoaded;

  /** electron mass */
  G4double fMassElectron;

  /** Total neutrino flux */
  G4double fTotalFlux;

  /** Random number generator for the nu spectrum sampler */
  CLHEP::RandGeneral *fSpectrumRndm;

  /** Name of the database entry to read the input spectrum from.
   * Defaults to SOLAR.
   */
  G4String fDBName;
};

}  // namespace RAT

#endif
