/** \file AmBeSource.cc
 *  AmBeSource C++ file.  Implements the constructor, copy
 *  constructor, and overloaded = operator and defines the
 *  AmBeNeutronSpectrum function.
 *
 *  Author: Vincent Fischer
 */
#include <CLHEP/Random/RandFlat.h>
#include <CLHEP/Random/RandGeneral.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <CLHEP/Vector/LorentzVector.h>

#include <G4Gamma.hh>
#include <G4Neutron.hh>
#include <G4ParticleDefinition.hh>
#include <RAT/AmBeSource.hh>
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <cmath>
#include <cstring>
#include <fstream>  // file I/O
#include <iomanip>  // format manipulation
#include <iostream>
#include <vector>

#undef DEBUG

namespace RAT {

double AmBeSource::massNeutron = 0.;  // allocate storage for static variable

AmBeSource::AmBeSource() {
  // Verify that all std::maps and std::vectors are empty.
  Nneutron = 0;
  Ngamma = 0;
  neutronE.clear();
  Tneutron.clear();
  gammaE.clear();
  Tgamma.clear();

  // setup the probability density as a function of energy

  // Random generators according to probability densities.
  static CLHEP::RandGeneral *fGenerate = 0;

  static const double flow = 0.;
  static const double fhigh = 12.;

  static bool first = true;
  if (first) {
    first = false;

    // Initialize the G4 particle definitions.
    G4ParticleDefinition *neutron = G4Neutron::Neutron();
    massNeutron = neutron->GetPDGMass() * CLHEP::MeV;

    // In the original code, the probability densities used the
    // funlxp and funlux routines in CERNLIB to generate random
    // numbers.  The following code uses CLHEP to generate the
    // same "histograms" for the RandGeneral random-number
    // generator.

    const size_t probDensSize = 200;
    double fspace[probDensSize];

#ifdef DEBUG
    debug << "AmBeSource initialization" << newline;
#endif

    // Find function values at bin centers.
    for (size_t i = 0; i != probDensSize; i++) {
      double value = (double(i) + 0.5) * (fhigh - flow) / (double)probDensSize;
      fspace[i] = AmBeNeutronSpectrum(value);
#ifdef DEBUG
      debug << "   i=" << i << ", value = " << value << " f,m,g=" << fspace[i] << newline;
#endif
    }

    // Define random-number generators.
    fGenerate = new CLHEP::RandGeneral(fspace, probDensSize);

#ifdef DEBUG
    debug << " Random generator test (f):" << newline;
    for (size_t i = 0; i != 20; i++) {
      debug << i << ": " << fGenerate->shoot() * (fhigh - flow) + flow << newline;
    }

#endif
  }

  // pick a neutron multiplicity
  Nneutron = 1;  // only one neutron generated

  // info << "   " << Nneutron << " neutrons" << newline;
  //
  //  pick a momentum direction for each neutron
  //
  for (int nn = 0; nn < Nneutron; nn++) {
    double neutronKE = fGenerate->shoot() * (fhigh - flow) + flow;
    // 	  info << "neutronKE = " << neutronKE*CLHEP::MeV << newline;
    double energy = massNeutron + neutronKE;
    // 	  info << "energy = " << energy*CLHEP::MeV << newline;
    // Generate momentum direction uniformly in phi and cos(theta).
    double phi = CLHEP::RandFlat::shoot(0., 2.0 * M_PI);
    double cosTheta = CLHEP::RandFlat::shoot(-1., 1.);
    double sinTheta = sqrt(1. - cosTheta * cosTheta);

    // Compute the momentum squared. If it comes out negative
    // due to roundoff errors, just set it equal to zero. This
    // prevents problems when we take the square root below.
    double neutronP2 = std::max(0., energy * energy - massNeutron * massNeutron);

    // Compute the momentum components
    double neutronP = std::sqrt(neutronP2);
    double px = neutronP * sinTheta * cos(phi);
    double py = neutronP * sinTheta * sin(phi);
    double pz = neutronP * cosTheta;
#ifdef DEBUG
    debug << "AmBeSource::AmBeSource() - neutron energy " << nn << " = " << energy << ", KE=" << neutronKE
          << ", (px,py,pz)=(" << px << "," << py << "," << pz << ")" << newline;
#endif
    CLHEP::HepLorentzVector momentum(px, py, pz, energy);
    neutronE.push_back(momentum);
    Tneutron.push_back(0.);
  }

  DBLinkPtr neutrondb = DB::Get()->GetLink("AMBE_NSPECTRUM", "");
  double prob_gamma_emission = neutrondb->GetD("prob_gamma_emission");
  double prob_gamma = CLHEP::RandFlat::shoot(0., 1.);
  if (prob_gamma < prob_gamma_emission) {
    Ngamma = 1;  // only 1 gamma generated
  } else {
    Ngamma = 0;  // no gamma generated
  }

#ifdef DEBUG
  debug << "AmBeSource::AmBeSource - "
        << "m=" << m << " => " << Ngamma << " photons" << newline;
#endif
  // pick a momentum for each gamma
  //
  double tote = 0.;
  for (int nn = 0; nn < Ngamma; nn++) {
    double energy = 4.43;  // from the C12 first excited state
    // Generate momentum direction uniformly in phi and cos(theta).
    double phi = CLHEP::RandFlat::shoot(0., 2.0 * M_PI);
    double cosTheta = CLHEP::RandFlat::shoot(-1., 1.);
    double sinTheta = sqrt(1. - cosTheta * cosTheta);
    double px = energy * sinTheta * cos(phi);
    double py = energy * sinTheta * sin(phi);
    double pz = energy * cosTheta;
#ifdef DEBUG
    debug << "AmBeSource::AmBeSource() - gamma energy " << nn << " = " << energy << ", (px,py,pz)=(" << px << "," << py
          << "," << pz << ")" << newline;
#endif
    CLHEP::HepLorentzVector momentum(px, py, pz, energy);
    gammaE.push_back(momentum);
    tote += energy;

    // consider the gammas is emitted instantaneously (10^-15 -ish decay time)
    Tgamma.push_back(0.);
  }
  // info << "          total energy = " << tote << newline;
}

AmBeSource::~AmBeSource() { ; }

AmBeSource::AmBeSource(const AmBeSource &_AmBeSource) {
  Nneutron = _AmBeSource.Nneutron;
  Ngamma = _AmBeSource.Ngamma;
  neutronE = _AmBeSource.neutronE;
  Tneutron = _AmBeSource.Tneutron;
  gammaE = _AmBeSource.gammaE;
  Tgamma = _AmBeSource.Tgamma;
}

AmBeSource &AmBeSource::operator=(const AmBeSource &rhs) {
  if (this != &rhs) {
    Nneutron = rhs.Nneutron;
    Ngamma = rhs.Ngamma;
    neutronE = rhs.neutronE;
    Tneutron = rhs.Tneutron;
    gammaE = rhs.gammaE;
    Tgamma = rhs.Tgamma;
  }
  return *this;
}

double AmBeSource::AmBeNeutronSpectrum(const double &x) {
  // return the neutron spectrum N(x)
  double N = 0.;

  DBLinkPtr neutrondb = DB::Get()->GetLink("AMBE_NSPECTRUM", "");
  std::vector<double> neutron_Espectrum_e = neutrondb->GetDArray("energy_spectrum");
  std::vector<double> neutron_Espectrum_p = neutrondb->GetDArray("energy_prob");
  const size_t size_arrays = neutron_Espectrum_e.size();

  for (size_t i = 0; i != size_arrays; i++) {
    // No extrapolation, the PDF goes to 0 at those boundaries anyway
    if (x < neutron_Espectrum_e[0] || x > neutron_Espectrum_e[size_arrays - 1]) {
      N = 0;
      break;
    }
    // If the requested value is exactly equal to a value present in the arrays
    if (x == neutron_Espectrum_e[i]) {
      N = neutron_Espectrum_p[i];
      break;
    }
    // Now the interpolation part
    if (x > neutron_Espectrum_e[i] && x < neutron_Espectrum_e[i + 1]) {
      N = neutron_Espectrum_p[i] + (x - neutron_Espectrum_e[i]) *
                                       (neutron_Espectrum_p[i + 1] - neutron_Espectrum_p[i]) /
                                       (neutron_Espectrum_e[i + 1] - neutron_Espectrum_e[i]);
      break;
    }
  }

  return N;
}

}  // namespace RAT
