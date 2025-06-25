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
#include <RAT/VertexGen_Laserball.hh>
#include <Randomize.hh>

namespace RAT {

VertexGen_Laserball::VertexGen_Laserball(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  fOpticalPhoton = G4ParticleTable::GetParticleTable()->FindParticle("opticalphoton");
  fWavelengthIndex = "";
  fNumPhotons = 0;
  fExpTime = 0;
}

void VertexGen_Laserball::GeneratePrimaryVertex(G4Event *event, G4ThreeVector &dx, G4double dt) {
  // Pick direction isotropically
  G4ThreeVector mom;
  double theta;
  double phi;
  double wavelength;
  double energy;
  DBLinkPtr spectradb;
  std::vector<double> wls;
  std::vector<double> wl_probs;

  try {
    spectradb = DB::Get()->GetLink("LASERBALL", fWavelengthIndex);
  } catch (DBNotFoundError &e) {
    Log::Die("VertexGen_Laserball: Wavelength simulated does not have measured spectrum.");
  }

  try {
    wls = spectradb->GetDArray("wavelength");
    wl_probs = spectradb->GetDArray("intensity");
    if (wls.size() != wl_probs.size()) {
      Log::Die("VertexGen_Laserball: Wavelength and probability arrays have different length");
    }
  } catch (DBNotFoundError &e) {
    Log::Die("VertexGen_Laserball: Error with retrieving wavelength from spectrum.");
  }

  for (size_t i = 0; i < fNumPhotons; i++) {
    theta = acos(2.0 * G4UniformRand() - 1.0);
    phi = 2.0 * G4UniformRand() * CLHEP::pi;

    wavelength = pickWavelength(wls, wl_probs);
    energy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);

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
  }
}

void VertexGen_Laserball::SetState(G4String newValues) {
  std::istringstream is(newValues.c_str());
  int num;
  std::string wavelength;
  is >> num >> wavelength;

  if (is.fail()) {
    Log::Die(
        "VertexGen_Laserball: Vertex state string incorrectly formatted. Format of argument to "
        "VertexGen_Laserball::SetState: \"num_photons wavelength[nm]nm\"\n");
  }
  fWavelengthIndex = wavelength;

  double exp = 0.0;
  is >> exp;
  if (exp < 0.0) {
    Log::Die("VertexGen_Laserball: Exponential time constant must be positive");
  }

  fNumPhotons = num;
  fExpTime = exp;
}

G4String VertexGen_Laserball::GetState() { return dformat("%d\t%f\t%f", fNumPhotons, fExpTime, fWavelengthIndex); }

double VertexGen_Laserball::pickWavelength(std::vector<double> &values, std::vector<double> &probs) {
  if (probs[0] != 0) {
    Log::Die("VertexGen_Laserball::pickWavelength: Leading entry in intensity is not 0");
  }
  double integral = 0.0;
  std::vector<double> probCumu = std::vector<double>(values.size());
  probCumu[0] = 0.0;

  for (size_t i = 0; i < values.size() - 1; i++) {
    if (probs[i] < 0) {
      Log::Die("VertexGen_Laserball::pickWavelength: An intensity is negative");
    } else if (i != 0 && probs[i] == 0) {
      Log::Die("VertexGen_Laserball::pickWavelength: Non-leading entry in intensity is 0");
    }
    integral += (values[i + 1] - values[i]) * (probs[i] + probs[i + 1]) / 2.0;  // trapezoid integration
    probCumu[i + 1] = integral;
  }
  for (size_t i = 0; i < values.size(); i++) {
    probs[i] /= integral;
    probCumu[i] /= integral;
  }

  double rval = G4UniformRand();

  // Check edge cases first

  if (rval < probCumu[1]) {
    return rval * (values[1] - values[0]) / (probCumu[1]) + values[0];
  } else if (rval == 1) {
    return values[values.size() - 1];
  } else {
    int low = 0;
    int high = probCumu.size() - 1;
    while (low <= high) {
      int mid = low + (high - low) / 2;

      // Check if x is present at mid
      if (rval <= probCumu[mid] && rval > probCumu[mid - 1]) {
        return (rval - probCumu[mid - 1]) * (values[mid] - values[mid - 1]) / (probCumu[mid] - probCumu[mid - 1]) +
               values[mid - 1];
      }
      // If x greater, ignore left half
      if (rval > probCumu[mid]) {
        low = mid + 1;
      }
      // If x is smaller, ignore right half
      else {
        high = mid - 1;
      }
    }
  }

  Log::Die("VertexGen_Laserball::pickWavelength: impossible condition encountered");
}

}  // namespace RAT