#include <CLHEP/Units/PhysicalConstants.h>

#include <RAT/DB.hh>
#include <RAT/IBDgen.hh>
#include <RAT/IBDgenMessenger.hh>

namespace RAT {

// Additional constants
const double DELTA = CLHEP::neutron_mass_c2 - CLHEP::proton_mass_c2;
const double GFERMI = 1.16639e-11 / CLHEP::MeV / CLHEP::MeV;

IBDgen::IBDgen() {
  SetPositronState(true);
  SetNeutronState(true);
  messenger = new IBDgenMessenger(this);
  SpectrumIndex = "default";
  UpdateFromDatabaseIndex();
}

void IBDgen::UpdateFromDatabaseIndex() {
  DBLinkPtr libd = DB::Get()->GetLink("IBD", SpectrumIndex);

  Emin = libd->GetD("emin");
  Emax = libd->GetD("emax");
  // Flux function
  rmpflux.Set(libd->GetDArray("spec_e"), libd->GetDArray("spec_flux"));
  std::vector<double> flux = libd->GetDArray("spec_flux");
  FluxMax = 1.1 * (*max_element(flux.begin(), flux.end()));
  // Other useful numbers
  XCmax = CrossSection(Emax, -1);
  // Do we use the cross section or is it included?
  ApplyCrossSection = libd->GetI("apply_xs");
}

void IBDgen::SetSpectrumIndex(G4String _specIndex) {
  SpectrumIndex = _specIndex;
  UpdateFromDatabaseIndex();
}

void IBDgen::GenEvent(const CLHEP::Hep3Vector &nu_dir, CLHEP::HepLorentzVector &neutrino,
                      CLHEP::HepLorentzVector &positron, CLHEP::HepLorentzVector &neutron) {
  double Enu, CosThetaLab;

  // Pick energy of neutrino and relative direction of positron
  GenInteraction(Enu, CosThetaLab);

  // First order correction to positron quantities
  // for finite nucleon mass
  double E1 = PositronEnergy(Enu, CosThetaLab);
  double p1 = sqrt(E1 * E1 - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2);

  // Compute nu 4-momentum
  neutrino.setVect(nu_dir * Enu);  // MeV (divide by c if need real units)
  neutrino.setE(Enu);

  // Compute positron 4-momentum
  CLHEP::Hep3Vector pos_momentum(p1 * nu_dir);

  // Rotation from nu direction to pos direction.
  double theta = acos(CosThetaLab);
  double phi = CLHEP::twopi * CLHEP::HepUniformRand();  // Random phi
  CLHEP::Hep3Vector rotation_axis = nu_dir.orthogonal();
  rotation_axis.rotate(phi, nu_dir);
  pos_momentum.rotate(theta, rotation_axis);

  positron.setVect(pos_momentum);
  positron.setE(E1);

  // Compute neutron 4-momentum
  neutron.setVect(neutrino.vect() - positron.vect());
  neutron.setE(sqrt(neutron.vect().mag2() + CLHEP::neutron_mass_c2 * CLHEP::neutron_mass_c2));
}

void IBDgen::GenInteraction(double &E, double &CosThetaLab) {
  bool passed = false;

  while (!passed) {
    // Pick E and cos(theta) uniformly
    E = Emin + (Emax - Emin) * CLHEP::HepUniformRand();
    CosThetaLab = -1.0 + 2.0 * CLHEP::HepUniformRand();

    if (ApplyCrossSection) {
      // Decided whether to draw again based on relative cross-section.
      double XCtest = XCmax * FluxMax * CLHEP::HepUniformRand();
      double XCWeight = CrossSection(E, CosThetaLab);
      double FluxWeight = rmpflux(E);
      passed = XCWeight * FluxWeight > XCtest;
    } else {
      // Decide whether to draw again based on relative dSigma/dCosT cross
      // section. Find the maximum of dE1/dCosT
      double dE1dCosTMax = EvalMax(E, FluxMax);
      double XCtest = dE1dCosTMax * CLHEP::HepUniformRand();
      double dEdCosTWeight = dE1dCosT(E, CosThetaLab);
      double FluxWeight = rmpflux(E);
      passed = dEdCosTWeight * FluxWeight > XCtest;
    }
  }
}

double IBDgen::CrossSection(double Enu, double CosThetaLab) {
  // returns dSigma/dCosTheta with first order corrections
  // Cross section constants.  Some for overall scale are just
  // to allow absolute comparison to published article.
  //
  const double CosThetaC = (0.9741 + 0.9756) / 2;
  //
  // Radiative correction constant
  //
  const double RadCor = 0.024;
  //
  // check for threshold
  //
  const double EminBeta = ((CLHEP::proton_mass_c2 + DELTA + CLHEP::electron_mass_c2) *
                               (CLHEP::proton_mass_c2 + CLHEP::electron_mass_c2 + DELTA) -
                           CLHEP::proton_mass_c2 * CLHEP::proton_mass_c2) /
                          2 / CLHEP::proton_mass_c2;

  if (Enu < EminBeta) return 0;

  //
  // overall scale
  //
  const double Sigma0 = GFERMI * GFERMI * CosThetaC * CosThetaC / CLHEP::pi * (1 + RadCor);
  //
  // couplings
  //
  const double f = 1.00;
  const double f2 = 3.706;
  const double g = 1.26;

  //
  // order 0 terms
  //
  double E0 = Enu - DELTA;
  if (E0 < CLHEP::electron_mass_c2) E0 = CLHEP::electron_mass_c2;
  double p0 = sqrt(E0 * E0 - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2);
  double v0 = p0 / E0;

  //
  //  order 1 terms
  //
  double E1 = PositronEnergy(Enu, CosThetaLab);
  double p1 = sqrt(E1 * E1 - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2);
  double v1 = p1 / E1;

  double Gamma =
      2 * (f + f2) * g *
          ((2 * E0 + DELTA) * (1 - v0 * CosThetaLab) - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2 / E0) +
      (f * f + g * g) * (DELTA * (1 + v0 * CosThetaLab) + CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2 / E0) +
      (f * f + 3 * g * g) * ((E0 + DELTA) * (1 - CosThetaLab / v0) - DELTA) +
      (f * f - g * g) * ((E0 + DELTA) * (1 - CosThetaLab / v0) - DELTA) * v0 * CosThetaLab;

  double XC =
      ((f * f + 3 * g * g) + (f * f - g * g) * v1 * CosThetaLab) * E1 * p1 - Gamma / CLHEP::proton_mass_c2 * E0 * p0;
  XC *= Sigma0 / 2 * CLHEP::hbarc * CLHEP::hbarc;  // Convert from MeV^-2 to mm^2 (native units for GEANT4)

  return XC;
}

double IBDgen::dE1dCosT(double Enu, double CosThetaLab) {
  // Returns dEe/dCosTheta with first order corrections
  // to positron quantities
  // Strumia & Vissani 2003 Equiation #20
  double epsilon = Enu / CLHEP::proton_mass_c2;
  double E1 = PositronEnergy(Enu, CosThetaLab);
  double p1 = sqrt(E1 * E1 - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2);
  double dE1_dCosT = p1 * epsilon / (1 + epsilon * (1 - CosThetaLab * E1 / p1));

  return dE1_dCosT;
}

double IBDgen::EvalMax(double Enu, double FluxMax) {
  // Finds the maximum value of dE1/dCosTheta
  // in range (-1,1)
  // always highest at cosT = 1
  double max = 0.;
  for (int i = 0; i < 200.; i++) {
    double cosT = (i - 100.) / 100.;
    double tmp = dE1dCosT(Enu, cosT) * FluxMax;
    if (tmp > max) max = tmp;
  }
  return max;
}

double IBDgen::PositronEnergy(double Enu, double CosThetaLab) {
  // Returns positron energy with first order corrections
  // Zero'th order approximation of positron quantities (infinite nucleon mass)
  double E0 = Enu - DELTA;
  double p0 = sqrt(E0 * E0 - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2);
  double v0 = p0 / E0;
  // First order correction to positron energy for finite nucleon mass
  const double Ysquared = (DELTA * DELTA - CLHEP::electron_mass_c2 * CLHEP::electron_mass_c2) / 2;
  double E1 = E0 * (1 - Enu / CLHEP::proton_mass_c2 * (1 - v0 * CosThetaLab)) - Ysquared / CLHEP::proton_mass_c2;
  if (E1 < CLHEP::electron_mass_c2) E1 = CLHEP::electron_mass_c2;

  return E1;
}

}  // namespace RAT
