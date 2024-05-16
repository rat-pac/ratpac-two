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

  // pick a gamma multiplicity
  // According to "Progress in Nuclear Science and Technology  - Volume 4 (2014)
  // pp. 345-348 (http://www.aesj.or.jp/publication/pnst004/data/345_348.pdf)
  // about 25% of Am-Be reactions do not emit a 4.43 MeV gamma (depends on the
  // energy level the carbon atom is excited to after the (alpha,n) reaction)
  // This probability can be changed here is you desire
  double prob_gamma_emission = 0.75;  // probability of emitted a gamma along with a neutron
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

  // Neutron energy spectrum of a AmBe source simulated with MCNP (NIM A Volume
  // 763, 1 November 2014, Pages 547-552)
  const size_t size_arrays = 93;
  // Neutron spectrum energy bins
  double neutron_Espectrum_e[size_arrays] = {
      0.01,   0.012,  0.015,  0.019,  0.023,  0.03,   0.037,  0.046,  0.058, 0.074, 0.093, 0.109, 0.144, 0.213,
      0.284,  0.351,  0.417,  0.485,  0.551,  0.626,  0.685,  0.756,  0.821, 0.892, 0.955, 1.029, 1.085, 1.152,
      1.233,  1.3,    1.36,   1.423,  1.489,  1.593,  1.692,  1.77,   1.824, 1.894, 1.952, 2.042, 2.089, 2.169,
      2.252,  2.321,  2.374,  2.447,  2.502,  2.54,   2.638,  2.698,  2.78,  2.844, 2.887, 2.952, 3.02,  3.112,
      3.135,  3.207,  3.33,   3.51,   3.728,  4.02,   4.174,  4.501,  4.781, 5.04,  5.273, 5.476, 5.816, 6.039,
      6.177,  6.511,  6.66,   7.127,  7.569,  8.162,  8.54,   8.935,  9.349, 9.278, 9.563, 9.784, 10.01, 10.241,
      10.478, 10.721, 10.726, 10.894, 10.901, 11.002, 10.927, 11.017, 11.026};

  // Neutron spectrum probability
  double neutron_Espectrum_p[size_arrays] = {
      0,     0,     0,     0.001, 0.001, 0.001, 0.001, 0.002, 0.002, 0.003, 0.004, 0.005, 0.007, 0.011, 0.016, 0.024,
      0.033, 0.042, 0.05,  0.055, 0.066, 0.075, 0.081, 0.088, 0.096, 0.098, 0.103, 0.109, 0.111, 0.114, 0.116, 0.111,
      0.113, 0.111, 0.118, 0.134, 0.153, 0.162, 0.174, 0.177, 0.191, 0.202, 0.214, 0.218, 0.234, 0.243, 0.262, 0.267,
      0.293, 0.316, 0.353, 0.395, 0.442, 0.476, 0.503, 0.563, 0.595, 0.606, 0.629, 0.641, 0.653, 0.653, 0.653, 0.677,
      0.69,  0.629, 0.552, 0.512, 0.531, 0.484, 0.458, 0.531, 0.583, 0.628, 0.652, 0.594, 0.541, 0.531, 0.449, 0.502,
      0.379, 0.252, 0.158, 0.111, 0.067, 0.038, 0.015, 0.007, 0.002, 0,     0,     0,     0,
  };

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
