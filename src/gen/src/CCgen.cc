// Generates a neutrino-nucleus charged current event, based on the
// cross-section as function of neutrino energy and the electron's
// recoil energy.  Allow for variations in the weak mixing angle and
// the possibility of a neutrino magnetic moment
//
// J. Formaggio (UW) -02/09/2005

// Converted to Geant4+GLG4Sim+RAT by Bill Seligman (07-Feb-2006).
// I'm following the code structure of the IBD classes:
// RATVertexGen_CC handles the G4-related tasks of constructing an
// event, while this class deals with the physics of the
// cross-section.  Some of the code (the flux in particular) is copied
// from IBDgen.

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <TF1.h>
#include <TGraph.h>
#include <TMath.h>

#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4ThreeVector.hh>
#include <RAT/CCCrossSec.hh>
#include <RAT/CCgen.hh>
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <Randomize.hh>
#include <cmath>

namespace RAT {

// This class is a helper to take care of the type of spectrum that is going to
// be used and reads the RATDB entries accordingly.

CCgen::CCgen()
    : fNuType(""),
      fNuFlavor(""),
      fXS(NULL),
      fNuSpectrum(NULL),
      fFluxMax(0.),
      fGenLoaded(false),
      fSpectrumRndm(0),
      fDBName("SOLAR") {
  // Initialize pointers
  fMassElectron = CLHEP::electron_mass_c2;

  // Load the default generator
  // Later, depending on the options passed this can be reloaded
  // Data needed to Load the stuff: type of spectrum (solar or ibd, flux and
  // flavor)
  LoadGenerator();
}

void CCgen::LoadGenerator() {
  // Check if the generator is already loaded.
  // If it is, do nothing
  if (fGenLoaded) return;
  if (fNuType.length() == 0 || fNuFlavor.length() == 0) {
    fGenLoaded = false;
    return;
  }

  // The parameters are taken from the database, depending of the option passed.
  // The original test with IBD data should still work
  DBLinkPtr linkdb;

  // Set default key names
  std::string enuMinKey = "emin";
  std::string enuMaxKey = "emax";
  std::string specEKey = "spec_e";
  std::string specFluxKey = "spec_flux";

  if (fDBName == "SOLAR") {
    // Solar generator
    // The nu type is obtained from the job options (it defaults to pep)
    linkdb = DB::Get()->GetLink(fDBName, fNuType);
    fTotalFlux = linkdb->GetD("flux");
  } else if (fDBName == "STPI") {
    // stopped pion generator
    // The nu type is the timing profile of the beam
    linkdb = DB::Get()->GetLink(fDBName, fNuType);
    fTotalFlux = linkdb->GetD("flux");
    if (fNuFlavor == "nue") {
      specFluxKey = "spec_flux_nue";
    } else if (fNuFlavor == "numu") {
      // Special monoenergetic case
      enuMinKey = "emin_numu";
      enuMaxKey = "emax_numu";
      specEKey = "spec_e_numu";
      specFluxKey = "spec_flux_numu";
    } else if (fNuFlavor == "numubar") {
      specFluxKey = "spec_flux_numubar";
    }
  } else {
    // should be IBD data
    linkdb = DB::Get()->GetLink(fDBName);
    fNuFlavor = "nuebar";
  }

  fEnuMin = linkdb->GetD(enuMinKey);
  fEnuMax = linkdb->GetD(enuMaxKey);
  fEnuTbl = linkdb->GetDArray(specEKey);
  fFluxTbl = linkdb->GetDArray(specFluxKey);

  // Check what type of CC generator we are dealing with
  // Depending on type the parameters and cross section pointers
  // are initialized differently

  fNuSpectrum = new TGraph(fEnuTbl.size(), &fEnuTbl[0], &fFluxTbl[0]);

  fFermiAngle = new TF1("fFermiAngle", "1+cos(x)", 0, TMath::Pi());
  fGTAngle = new TF1("fGTAngle", "1-1.0/3*cos(x)", 0, TMath::Pi());

  // initialize the cross-section
  if (fXS != 0) {
    delete fXS;
  }
  fXS = new CCCrossSec(fNuFlavor);

  int last_xs_zero = -1;
  unsigned int last_flux_zero = fFluxTbl.size();
  // To sample neutrino energy need to scale flux by total
  // cross section at that neutrino energy
  std::vector<double> csScaledFluxTbl(fFluxTbl.size(), 0);
  for (size_t i = 0; i < csScaledFluxTbl.size(); i++) {
    double ccxs = fXS->Sigma(fEnuTbl[i]);
    csScaledFluxTbl[i] = fFluxTbl[i] * ccxs;
    if (ccxs == 0) {
      last_xs_zero = i;
    }
    if (fFluxTbl[i] == 0) {
      last_flux_zero = i;
    }
  }

  // Cast is a little dangerous but should be ok
  if (last_xs_zero != -1 || last_flux_zero != fFluxTbl.size()) {
    fEnuMin = fEnuTbl[last_xs_zero + 1];
    fEnuMax = fEnuTbl[last_flux_zero - 1];
    std::vector<double> temp;
    for (unsigned int i = last_xs_zero + 1; i < last_flux_zero; i++) {
      temp.push_back(csScaledFluxTbl[i]);
    }
    csScaledFluxTbl = temp;
  }

  // If random sampler hasn't been initialized yet, lets do it now
  if (!fSpectrumRndm) {
    // Be7 is always a particular case due to its discrete nature
    // The last parameter is set to 1 to disallow interpolations
    if (fNuType == "be7") {
      fSpectrumRndm = new CLHEP::RandGeneral(&csScaledFluxTbl[0], csScaledFluxTbl.size(), 1);
    } else {
      // set interpolation bit to 0 to allow for interpolations in continuous
      // spectra
      fSpectrumRndm = new CLHEP::RandGeneral(&csScaledFluxTbl[0], csScaledFluxTbl.size(), 0);
    }
  }

  // If it reaches this point without failing then everything should be fine
  fGenLoaded = true;
  info << "Rate per target for CC of " << fNuType.c_str() << " flux on Li7 is: " << GetRatePerTarget() << newline;
}

CCgen::~CCgen() {
  if (fXS != 0) {
    delete fXS;
    fXS = 0;
  }

  if (fNuSpectrum != 0) {
    delete fNuSpectrum;
    fNuSpectrum = 0;
  }

  if (fSpectrumRndm) {
    delete fSpectrumRndm;
    fSpectrumRndm = 0;
  }

  if (fFermiAngle) {
    delete fFermiAngle;
    fFermiAngle = 0;
  }

  if (fGTAngle) {
    delete fGTAngle;
    fGTAngle = 0;
  }
}

void CCgen::GenerateEvent(const G4ThreeVector &theNeutrino, G4LorentzVector &nu_incoming, G4LorentzVector &electron,
                          double &e_nucleus) {
  // Check if the generator has been loaded successfully
  // For now just throw something that can be caught at an upper level.
  // Need to define a set of specific exceptions
  if (!fGenLoaded) {
    G4Exception("[CCgen]::GenerateEvent", "ArgError", FatalErrorInArgument,
                "Vertex generation called but it seems that it is not ready yet.");
  }

  ///!
  ///! Have to be careful with the line neutrino types (pep)
  ///! and even more careful with the double line (be7)
  ///!

  // Throw values against a cross-section.
  // bool passed=false;
  double Enu, Te, Enucleus;
  int TransitionType;
  // Updated sampler (orders of magnitude faster)
  // Given the neutrino energy, use the differential cross section
  // shape and sample from it.
  Enu = SampleNuEnergy() * CLHEP::MeV;
  Te = SampleRecoilEnergy(Enu, TransitionType, Enucleus) * CLHEP::MeV;
  // Doesn't support exictation currently
  e_nucleus = Enucleus;

  // from the incoming neutrino we have already the initial direction.
  // The final electron direction will follow that.

  // Now we have :
  // - the initial direction of the neutrino (unnormalized)
  // - the energy of the neutrino
  // - the recoil energy of the electron

  // build the 4-momentum std::vector of the neutrino
  // We will only use the neutrino initial momentum as a baseline to add up to
  // the electron direction

  G4double theta_e = SampleRecoilAngle(Enu, Te, TransitionType);

  G4double tot_Ee = Te + fMassElectron;
  G4double p_e = sqrt(tot_Ee * tot_Ee - fMassElectron * fMassElectron);

  // We have theta. Now randomly generate phi
  G4double phi_e = CLHEP::twopi * G4UniformRand();

  // This is the incoming neutrino information
  nu_incoming.setVect(theNeutrino * Enu);
  nu_incoming.setE(Enu);

  // compute electron 4-momentum
  G4ThreeVector e_mom(p_e * theNeutrino);

  // Rotation from nu direction into electron direction
  G4ThreeVector rotation_axis = theNeutrino.orthogonal();
  rotation_axis.rotate(phi_e, theNeutrino);
  e_mom.rotate(theta_e, rotation_axis);
  electron.setVect(e_mom);
  electron.setE(tot_Ee);

  // TODO: If we want to keep track of the outgoing neutrino have to add it
  // here as well and store the information in a new argument G4LorentzVector
  // variable For the moment we only pass the incoming neutrino information
  // back.
}

void CCgen::Reset() {
  // Reset the falg dependent objects.
  // After this method a call to LoadGenerator should always follow
  if (fNuSpectrum) {
    delete fNuSpectrum;
    fNuSpectrum = 0;
  }
  if (fSpectrumRndm) {
    delete fSpectrumRndm;
    fSpectrumRndm = 0;
  }

  LoadGenerator();
}

void CCgen::Show() {
  info << "Charged Current Settings:" << newline;
  info << "NuType : " << fNuType.c_str() << newline;
}

//
// If we change the neutrino type we should reload the generator
// to force it to reload the spectra from the database
void CCgen::SetNuType(const G4String &nutype) {
  if (fNuType != nutype) {
    fNuType = nutype;
    fGenLoaded = false;
    Reset();
    LoadGenerator();
  }
}

//
// If we change the neutrino flavor we should reload the generator
// to force it to reload the spectra from the database
void CCgen::SetNuFlavor(const G4String &nuflavor) {
  if (fNuFlavor != nuflavor) {
    fNuFlavor = nuflavor;
    fGenLoaded = false;
    Reset();
    LoadGenerator();
  }
}

void CCgen::SetDBName(const G4String name) {
  if (fDBName != name) {
    fDBName = name;
    fGenLoaded = false;
    Reset();
    LoadGenerator();
  }
}

// This function samples the energy spectrum of the chosen neutrino and
// decides from it the proper energy.
// Keep in mind that pep is always the same, but be7 is a *very* special case
G4double CCgen::SampleRecoilEnergy(G4double Enu, int &Transition, double &Enucleus) {
  G4double Te = 0.0;

  // Get the shape of the differential cross section.
  std::vector<double> scaled_norms = fXS->CalcdSigmadTNorms(Enu);
  std::vector<double> allowed_ke = fXS->CalcAllowedElectronKE(Enu);
  std::vector<double> allowed_nuclear = fXS->CalcAllowedNuclearEx(Enu);

  double total = 0;
  for (unsigned int i = 0; i < scaled_norms.size(); i++) {
    total += scaled_norms[i];
  }
  std::vector<double> cumulative;
  for (unsigned int i = 0; i < scaled_norms.size(); i++) {
    cumulative.push_back(scaled_norms[i] * 1.0 / total);
    if (i > 0) {
      cumulative[i] += cumulative[i - 1];
    }
  }

  double rand = CLHEP::RandFlat::shoot();
  int count = 0;
  double prev_cumul = 0;
  for (unsigned int i = 0; i < cumulative.size(); i++) {
    count = i;
    if (rand > prev_cumul && rand < cumulative[i]) {
      break;
    }
    prev_cumul = cumulative[i];
  }

  Te = allowed_ke[count];
  Transition = fXS->GetAllowedTransitionTypes(Enu)[count];
  Enucleus = allowed_nuclear[count];
  return Te;
}

G4double CCgen::SampleRecoilAngle(G4double Enu, G4double Te, int Transition) {
  G4double theta = 0;

  if (Transition == 0) {
    theta = fFermiAngle->GetRandom();
  } else if (Transition == 1) {
    theta = fGTAngle->GetRandom();
  }
  return theta;
}

// This function samples the energy spectrum of the chosen neutrino and
// decides from it the proper energy.
// Keep in mind that pep is always the same, but be7 is a *very* special case
G4double CCgen::SampleNuEnergy() {
  G4double Enu = 0.0;
  G4double tmp = 0.0;

  if (fNuType == G4String("pep")) {
    // This is a line flux. Only 1 energy possible.
    Enu = fEnuMin;
  } else if (fNuType == G4String("be7")) {
    // The neutrino energy of be7 must follow the branching ratio.
    // Otherwise the flux is not properly sampled.
    // Use CLHEP RandGeneral generator, with interpolation disabled to build
    // a discrete distribution of two states.
    // The random generator will return either 0 or 0.5
    tmp = fSpectrumRndm->shoot();
    Enu = (tmp < 0.5) ? fEnuMin : fEnuMax;
  } else {
    // Continuous distributions will return a random number between 0 and 1
    // Following the spectrum shape.
    // Scale it to the energy of the spectrum
    double scale = fEnuMax - fEnuMin;
    Enu = fEnuMin + fSpectrumRndm->shoot() * scale;
  }

  return Enu;
}

G4double CCgen::GetRatePerTarget() {
  if (!fGenLoaded) {
    G4Exception("CCgen::GetRatePerTarget", "404", FatalErrorInArgument,
                "Trying to get rate before initializing all parameters.");
  }

  G4double intRate = 0.0;

  // Have to deal with the special cases separately
  if (fNuType == "pep") {
    intRate = fXS->Sigma(fEnuMin);
  } else if (fNuType == "be7") {
    intRate = fXS->Sigma(fEnuMin) * fNuSpectrum->GetY()[0];
    intRate += fXS->Sigma(fEnuMax) * fNuSpectrum->GetY()[1];
  } else {
    // For some unknown reason ROOT TGraph::Integral() does not
    // like to calculate integrals of adjacent points
    // so I do it by hand using a trapezoidal rule
    // The input spectra have a fine enough grain that the error
    // is negligible.

    for (G4int ip = 0; ip < fNuSpectrum->GetN() - 1; ++ip) {
      G4double de = fNuSpectrum->GetX()[ip + 1] - fNuSpectrum->GetX()[ip];
      G4double integ = (0.5 * de) * (fNuSpectrum->GetY()[ip + 1] * fXS->Sigma((fNuSpectrum->GetX())[ip + 1]) +
                                     fNuSpectrum->GetY()[ip] * fXS->Sigma((fNuSpectrum->GetX())[ip]));
      intRate += integ;
    }
  }
  info << "Interaction rate: " << intRate << " XS norm: " << fXS->CrossSecNorm() << " Total Flux: " << fTotalFlux
       << newline;
  // don't forget the scale factor from the cross section
  // nor the total flux
  return intRate * fXS->CrossSecNorm() * fTotalFlux;
}
}  // namespace RAT
