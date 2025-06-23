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

using namespace RAT;
using namespace std;

VertexGen_Laserball::VertexGen_Laserball(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  fOpticalPhoton = G4ParticleTable::GetParticleTable()->FindParticle("opticalphoton");
  fMaterial = "";
  fWavelengthIndex = "";
  fNumPhotons = 0;
  fExpTime = 0;
  SetState("1 408nm");  // one photon per event, 408 nm
}

VertexGen_Laserball::~VertexGen_Laserball() {}

void VertexGen_Laserball::GeneratePrimaryVertex(G4Event *event, G4ThreeVector &dx, G4double dt) {
  // Pick direction isotropically
  G4ThreeVector mom;
  double theta;
  double phi;
  double wavelength;
  double energy;

  for (int i = 0; i < fNumPhotons; i++) {
    theta = acos(2.0 * G4UniformRand() - 1.0);
    phi = 2.0 * G4UniformRand() * CLHEP::pi;
    DBLinkPtr spectradb;
    try {
      spectradb = DB::Get()->GetLink("WLSPECTRA", fWavelengthIndex);
    } catch (DBNotFoundError &e) {
      Log::Die("VertexGen_Laserball: Wavelength simulated does not have measured spectrum.");
    }

    try {
      double wl_average = spectradb->GetD("avg_wl");
      std::vector<double> wls = spectradb->GetDArray("dist_wl");
      std::vector<double> wl_probs = spectradb->GetDArray("dist_wl_intensity");
      if (wls.size() != wl_probs.size()) {
        Log::Die("VertexGen_Laserball: Wavelength and probability arrays have different length");
      }
      wavelength = pickValue(wls, wl_probs, wl_average);
      energy = CLHEP::hbarc * CLHEP::twopi / (wavelength * CLHEP::nm);
    } catch (DBNotFoundError &e) {
      Log::Die("VertexGen_Laserball: Error with retrieving wavelength from spectrum.");
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
  }
}

void VertexGen_Laserball::SetState(G4String newValues) {
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this VertexGen_Laserball:" << newline << " \"" << GetState() << "\"" << newline
         << newline;
    info << "Format of argument to VertexGen_Laserball::SetState: " << newline << " \"num_photons wavelength_nm\"\n"
         << newline;
    return;
  }
  istringstream is(newValues.c_str());
  int num;
  string wavelength;
  is >> num >> wavelength;

  if (is.fail()) {
    Log::Die("VertexGen_Laserball: Vertex state string incorrectly formatted.");
  } else {
    fWavelengthIndex = wavelength;
  }

  double exp = 0.0;
  is >> exp;
  if (exp < 0.0) {
    Log::Die("VertexGen_Laserball: Exponential time constant must be positive");
  }

  fNumPhotons = num;
  fExpTime = exp;
}

G4String VertexGen_Laserball::GetState() { return dformat("%d\t%f\t%f", fNumPhotons, fExpTime, fWavelengthIndex); }

double VertexGen_Laserball::pickValue(vector<double> values, vector<double> probs, double avg) {
  double integral = 0.0;
  vector<double> probCumu = vector<double>(values.size());
  probCumu[0] = 0.0;
  for (size_t i = 0; i < values.size() - 1; i++) {
    integral += (values[i + 1] - values[i]) * (probs[i] + probs[i + 1]) / 2.0;  // trapezoid integration
    probCumu[i + 1] = integral;
  }
  for (size_t i = 0; i < values.size(); i++) {
    probs[i] /= integral;
    probCumu[i] /= integral;
  }
  double rval = G4UniformRand();
  for (size_t i = 1; i < values.size(); i++) {
    if (rval <= probCumu[i]) {
      return (rval - probCumu[i - 1]) * (values[i] - values[i - 1]) / (probCumu[i] - probCumu[i - 1]) +
             values[i - 1];  // linear interpolation
    }
  }
  Log::Die("VertexGen_Laserball::pickValue: impossible condition encountered");
  return avg;
}
