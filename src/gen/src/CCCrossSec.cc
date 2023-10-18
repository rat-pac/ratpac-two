// CCCrossSec.cc
// Contact person: Max Smiley <masmiley@berkeley.edu>
// See CCCrossSec.hh for more details
// ———————————————————————//

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <TH1F.h>
#include <TMath.h>

#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4PhysicalConstants.hh>
#include <RAT/CCCrossSec.hh>
#include <RAT/CCCrossSecMessenger.hh>
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <vector>

namespace RAT {

/**
// Couple of constants
// For consistency the values are taken from Geant4 directly
// except for Fermi's constant (fGf) which for some reason
// I couldn't find the reference
// Some constants have a constant multiplicative term
// That's to get the values into the correct units expected
// in the code, which are different from the ones
// provided from Geant4/CLHEP
 */

const double RAT::CCCrossSec::fGf = 1.166371e-11;                 // Fermi constant (MeV^-2)
const double RAT::CCCrossSec::fhbarc = CLHEP::hbarc * 1e12;       // hbar*c (MeV*fm)
const double RAT::CCCrossSec::fhbarc2 = fhbarc * fhbarc * 1e-11;  // hbar*c^2(MeV^2 mb)
const double RAT::CCCrossSec::falpha = CLHEP::fine_structure_const;

/**
 * The weak mixing angle (\f$ \sin^{2}\theta_{W}\f$) is declared only as static
 * (no const modifier) as it can be changed by macro command.
 * This change should reflect all instances of the class.
 */
double RAT::CCCrossSec::fsinthetaW2 = 0.23116;

CCCrossSec::CCCrossSec(const char *flavor) {
  Defaults();

  // Messenger to override some parameters
  fMessenger = new CCCrossSecMessenger(this);
}

CCCrossSec::~CCCrossSec() {
  if (fMessenger != NULL) {
    delete fMessenger;
    fMessenger = NULL;
  }
}

void CCCrossSec::Defaults() {
  // load default parameters
  // defaults to PDG value.
  // Can be tuned in the macro file or in the command
  // file through the ESxsectionMessenger
  // /generator/es/xsection/wma
  // fsinthetaW2 = 0.23116; // effective angle PDG 2010

  fMe = (G4ParticleTable::GetParticleTable()->FindParticle("e-"))->GetPDGMass();  // MeV

  fLevels.push_back(0.350);
  fLevelTypes.push_back(0);
  fLevels.push_back(0.350);
  fLevelTypes.push_back(1);
  fLevels.push_back(0.350 + 0.4291);
  fLevelTypes.push_back(1);
  fLevels.push_back(0.350 + 6.73);
  fLevelTypes.push_back(1);
  fLevels.push_back(0.350 + 7.21);
  fLevelTypes.push_back(1);
  fNorms.push_back(1.0);
  fNorms.push_back(1.7471);
  fNorms.push_back(1.6303);
  fNorms.push_back(0.01135);
  fNorms.push_back(0.07317);
}

//-------------------------------------------------------------------

/// Calculates total cross section for a given neutrino energy
double CCCrossSec::Sigma(const double Enu) const {
  // return total cross section in units cm^-42
  // for laboratory neutrino energy Enu
  // The calculation varies according to the chosen strategy

  // First do the most basic check
  if (Enu < 0) {
    std::stringstream ss;
    ss << "[CCCrossSec]::Sigma : Invalid neutrino Energy ( Enu = " << Enu << " ).";
    RAT::Log::Die(ss.str(), 1);
    throw std::invalid_argument("Invalid neutrino energy (" + ss.str() + ").");
  }

  double sigma = 0.0;

  /// double norm[5] = {1.0, 1.7471, 1.6303, 0.01135, 0.07317};
  /// double level[5];
  /// level[0] = 0.350;
  /// level[1] = 0.350;
  /// level[2] = 0.350+0.4291;
  /// level[3] = 0.350+6.73;
  /// level[4] = 0.350+7.21;

  double cos2thc = pow(0.97425, 2);
  double coeff = -1.0 * (8.0 * falpha) * fGf * fGf * cos2thc;
  double units = 0.389379 * 1e6 * 1e-27 * 1e42;  // hbar^2c^2 so when multiplied by below, get cm^2 unit
                                                 // for cross section. 1e42 is to get into units of 10^-42
                                                 // cm^2, which is what is expected

  double term[5];
  for (unsigned int n = 0; n < fLevels.size(); n++) {
    term[n] = 0.;
    double energy = Enu - fLevels[n];
    // Energy left over must be enough to create electron
    //(integral over Ee starts at 0.511 NOT 0 so must have value in d-fcn be
    // above this otherwise term is 0)
    if (energy > fMe) {
      double mom = pow((pow(energy, 2) - pow(fMe, 2)), 0.5);
      double expt = -1.0 * (8.0 * TMath::Pi() * falpha) * energy / mom;
      term[n] = units * coeff * fNorms[n] * (pow(energy, 2) / (TMath::Exp(expt) - 1.0));
      sigma += term[n];
    }
  }
  return sigma;
  // Should never reach this point
  // Throw an exception if that happens
  std::stringstream ss;
  ss << "[CCCrossSec]::Sigma : Reached end of function while calculating "
        "Sigma. Something is wrong with the calculation.\n";

  ss << "[CCCrossSec]::Sigma : Enu : [ " << Enu << "].";
  RAT::Log::Die(ss.str(), 1);
  throw;
}

std::vector<double> CCCrossSec::CalcAllowedElectronKE(const double Enu) const {
  std::vector<double> allowed_ke;
  // info << "Neutrino energy: " << Enu << newline;
  for (unsigned int n = 0; n < fLevels.size(); n++) {
    double energy = Enu - fLevels[n];
    if (energy > fMe) {
      // info << "Allowed e- kinetic energy: " << energy - fMe <<
      // newline;
      allowed_ke.push_back(energy - fMe);
    }
  }
  return allowed_ke;
}

std::vector<double> CCCrossSec::CalcAllowedNuclearEx(const double Enu) const {
  std::vector<double> allowed_nuclear;
  // info << "Neutrino energy: " << Enu << newline;

  for (unsigned int n = 0; n < fLevels.size(); n++) {
    double energy = Enu - fLevels[n];
    if (energy > fMe) {
      // info << "Allowed nuclear excitation: " << fLevels[n] - 0.350 <<
      // newline;
      allowed_nuclear.push_back(fLevels[n] - 0.350);
    }
  }
  return allowed_nuclear;
}

std::vector<double> CCCrossSec::GetAllowedTransitionTypes(const double Enu) const {
  std::vector<double> allowed_types;

  for (unsigned int n = 0; n < fLevels.size(); n++) {
    double energy = Enu - fLevels[n];
    if (energy > fMe) {
      allowed_types.push_back(fLevelTypes[n]);
    }
  }
  return allowed_types;
}

std::vector<double> CCCrossSec::CalcdSigmadTNorms(const double Enu) const {
  std::vector<double> scaled_norms;
  // double norm[5] = {1.0, 1.7471, 1.6303, 0.01135, 0.07317};
  // double level[5];
  // level[0] = 0.350;
  // level[1] = 0.350;
  // level[2] = 0.350+0.4291;
  // level[3] = 0.350+6.73;
  // level[4] = 0.350+7.21;

  for (unsigned int n = 0; n < fLevels.size(); n++) {
    double energy = Enu - fLevels[n];
    // Energy left over must be enough to create electron
    //(integral over Ee starts at 0.511 NOT 0 so must have value in d-fcn be
    // above this otherwise term is 0)
    if (energy > fMe) {
      double mom = pow((pow(energy, 2) - pow(fMe, 2)), 0.5);
      double expt = -1.0 * (8.0 * TMath::Pi() * falpha) * energy / mom;
      scaled_norms.push_back(fNorms[n] * (pow(energy, 2) / (TMath::Exp(expt) - 1.0)));
    }
  }
  return scaled_norms;
}

}  // namespace RAT
