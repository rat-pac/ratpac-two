#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Event.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/LinearInterp.hh>
#include <RAT/Log.hh>
#include <RAT/VertexGen_PhotonBomb.hh>
#include <Randomize.hh>

namespace RAT {
VertexGen_PhotonBomb::VertexGen_PhotonBomb(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  fOpticalPhoton = G4ParticleTable::GetParticleTable()->FindParticle("opticalphoton");
  SetState("1 400");  //  one photon per event, 400 nm, don't use distribution
  fRndmEnergy = 0;
  fMinEnergy = 0.0;
  fMaxEnergy = 0.0;
  fMaterial = "";
}

VertexGen_PhotonBomb::~VertexGen_PhotonBomb() { delete fRndmEnergy; }

void VertexGen_PhotonBomb::GeneratePrimaryVertex(G4Event *event, G4ThreeVector &dx, G4double dt) {
  // Pick direction isotropically
  G4ThreeVector mom;
  double theta;
  double phi;
  double wavelength;

  // Use fixed energy unless spectrum was provided, fDist refers to spectrum sampled on 01/27/25
  double energy;

  for (int i = 0; i < fNumPhotons; i++) {
    theta = acos(2.0 * G4UniformRand() - 1.0);
    phi = 2.0 * G4UniformRand() * CLHEP::pi;
    if (fRndmEnergy) {
      energy = fMinEnergy + (fMaxEnergy - fMinEnergy) * fRndmEnergy->shoot();
    } else if (fDist) {
      try {
        DBLinkPtr spectradb = DB::Get()->GetLink("LBSPECTRA", std::to_string((int)fWavelength));
        double wl_average = spectradb->GetD("avg_wl");
        wavelength = wl_average;
        try {
          std::vector<double> wls = spectradb->GetDArray("dist_wl");
          std::vector<double> wl_probs = spectradb->GetDArray("dist_wl_intensity");
          if (wls.size() != wl_probs.size())
            Log::Die("Wavelength Selection: wavelength and probability arrays of different length");
          wavelength = pickWavelength(wls, wl_probs, wl_average);
        } catch (DBNotFoundError &e) {
        }
        energy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);
      } catch (DBNotFoundError &e) {
        wavelength = fWavelength;
        energy = fEnergy;
      }
    } else {
      wavelength = fWavelength;
      energy = fEnergy;
    }
    mom.setRThetaPhi(energy, theta, phi);  // Momentum == energy units in GEANT4
    // Distribute times expoenentially, but don't bother picking a
    // random number if there is no time constant
    double expt = 0.0;
    if (fExpTime > 0.0) expt = -fExpTime * log(G4UniformRand());

    G4PrimaryVertex *vertex = new G4PrimaryVertex(dx, dt + expt);
    G4PrimaryParticle *photon = new G4PrimaryParticle(fOpticalPhoton, mom.x(), mom.y(), mom.z());
    // Generate random polarization
    phi = (G4UniformRand() * 2.0 - 1.0) * CLHEP::pi;
    G4ThreeVector e1 = mom.orthogonal().unit();
    G4ThreeVector e2 = mom.unit().cross(e1);
    G4ThreeVector pol = e1 * cos(phi) + e2 * sin(phi);
    photon->SetPolarization(pol.x(), pol.y(), pol.z());
    photon->SetMass(0.0);  // Seems odd, but used in GLG4VertexGen_Gun

    vertex->SetPrimary(photon);
    event->AddPrimaryVertex(vertex);
  }  // loop over photons
}

void VertexGen_PhotonBomb::SetState(G4String newValues) {
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this VertexGen_PhotonBomb:" << newline << " \"" << GetState() << "\"" << newline
         << newline;
    info << "Format of argument to VertexGen_PhotonBomb::SetState: " << newline << " \"num_photons wavelength_nm\"\n"
         << newline;
    return;
  }

  std::istringstream is(newValues.c_str());
  double num, wavelength;
  is >> num >> wavelength;

  if (is.fail()) {
    // check for scintillation wavelength spectrum
    is.str(newValues.c_str());
    is.clear();
    std::string material;
    is >> num >> material;
    if (is.fail()) Log::Die("VertexGen_PhotonBomb: Incorrect vertex setting " + newValues);
    fMaterial = material;

    // get the scintillation wavelength spectrum
    DBLinkPtr loptics = DB::Get()->GetLink("OPTICS", material);
    std::vector<double> wlarr = loptics->GetDArray("SCINTILLATION_value1");
    std::vector<double> wlamp = loptics->GetDArray("SCINTILLATION_value2");
    for (unsigned i = 0; i < wlarr.size(); i++) wlarr[i] = CLHEP::hbarc * CLHEP::twopi / (wlarr[i] * CLHEP::nm);
    if (wlarr.front() > wlarr.back()) {
      reverse(wlarr.begin(), wlarr.end());
      reverse(wlamp.begin(), wlamp.end());
    }
    for (unsigned i = 1; i < wlarr.size(); i++)
      if (wlarr[i - 1] >= wlarr[i]) Log::Die("VertexGen_PhotonBomb: wavelengths out of order");

    // use a linear interpolator to get a uniform sampling with bin
    // size smaller than the smallest bin in order to use RandGeneral
    LinearInterp<double> energyInterp(wlarr, wlamp);
    double step = 1.0e9;
    for (int i = 0; i < (int)wlarr.size() - 1; i++) step = fmin(step, wlarr[i + 1] - wlarr[i]);
    step /= 2;
    int nbins = (int)((energyInterp.Max() - energyInterp.Min()) / step) + 1;
    step = (energyInterp.Max() - energyInterp.Min()) / (nbins - 1);

    // get the oversampled array, small padding at ends to avoid range error
    double *energyResample = new double[nbins];
    energyResample[0] = energyInterp(energyInterp.Min() + step * 1e-6);
    energyResample[nbins - 1] = energyInterp(energyInterp.Max() - step * 1e-6);
    for (int i = 1; i < nbins - 1; i++) energyResample[i] = energyInterp(energyInterp.Min() + i * step);
    fMinEnergy = energyInterp.Min();
    fMaxEnergy = energyInterp.Max();

    if (fRndmEnergy) delete fRndmEnergy;
    fRndmEnergy = new CLHEP::RandGeneral(energyResample, nbins);
  } else {
    fEnergy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);
    fWavelength = wavelength;
  }

  bool dist = false;
  double testWavelength;

  try {
    DBLinkPtr spectraparams = DB::Get()->GetLink("LBSPECTRA", "params");
    dist = spectraparams->GetZ("include_dist");
    if (dist) {
      info << "Wavelength spectra rat table found, continuing with distribution" << newline;
    } else {
      info << "Wavelength spectra rat table found, continuing with nominal wavelength instead of distribution"
           << newline;
      fWavelength = wavelength;
      fEnergy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);
    }
  } catch (DBNotFoundError &e) {
    warn << "Wavelength spectra rat table could not be found, continuing with nominal wavelength" << newline;
    fWavelength = wavelength;
    fEnergy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);
  }

  try {
    DBLinkPtr spectradb = DB::Get()->GetLink("LBSPECTRA", std::to_string((int)fWavelength));
    testWavelength = spectradb->GetD("avg_wl");
    debug << "Successfully found the wavelength spectrum for " << std::to_string((int)fWavelength) << " nm photon bomb"
          << newline;
  } catch (DBNotFoundError &e) {
    debug << "Spectrum for" << std::to_string((int)fWavelength)
          << " not loaded correctly, continuing with nominal wavelength" << newline;
  }
  fDist = dist;

  double exp = 0.0;
  is >> exp;
  if (exp < 0.0) Log::Die("VertexGen_PhotonBomb: Exponential time constant must be positive");

  fNumPhotons = num;
  fExpTime = exp;
}

G4String VertexGen_PhotonBomb::GetState() {
  if (fRndmEnergy)
    return dformat("%d\t%s\t%f", fNumPhotons, fMaterial.c_str(), fExpTime);
  else
    return dformat("%d\t%f\t%f", fNumPhotons, fEnergy, fExpTime);
}

double VertexGen_PhotonBomb::pickWavelength(std::vector<double> wavelengths, std::vector<double> probs, double avg) {
  double integral = 0.0;
  std::vector<double> probCumu = std::vector<double>(wavelengths.size());
  probCumu[0] = 0.0;
  for (size_t i = 0; i < wavelengths.size() - 1; i++) {
    integral += (wavelengths[i + 1] - wavelengths[i]) * (probs[i] + probs[i + 1]) / 2.0;  // trapezoid integration
    probCumu[i + 1] = integral;
  }
  for (size_t i = 0; i < wavelengths.size(); i++) {
    probs[i] /= integral;
    probCumu[i] /= integral;
  }
  double rval = G4UniformRand();
  for (size_t i = 1; i < wavelengths.size(); i++) {
    if (rval <= probCumu[i]) {
      return (rval - probCumu[i - 1]) * (wavelengths[i] - wavelengths[i - 1]) / (probCumu[i] - probCumu[i - 1]) +
             wavelengths[i - 1];  // linear interpolation
    }
  }
  debug << "VertexGen::pickWavelength: impossible condition encountered - returning mean wavelength" << newline;
  return avg;
}

}  // namespace RAT
