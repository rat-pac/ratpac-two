// Calculates the cross-section for neutrino-nucleus charge current interaction
// as function of neutrino energy and the electron's recoil energy.
// Allow for variations in the weak mixing angle and the possibility
// of a neutrino magnetic moment
//

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Vector/LorentzVector.h>

#include <G4Event.hh>
#include <G4IonTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4ThreeVector.hh>
#include <RAT/CCgen.hh>
#include <RAT/GLG4PosGen.hh>
#include <RAT/GLG4StringUtil.hh>
#include <RAT/Log.hh>
#include <RAT/PrimaryVertexInformation.hh>
#include <RAT/StringUtil.hh>
#include <RAT/VertexGen_CC.hh>
#include <Randomize.hh>
#include <cmath>
#include <globals.hh>
#include <sstream>

namespace RAT {

VertexGen_CC::VertexGen_CC(const char *arg_dbname)
    : GLG4VertexGen(arg_dbname), fNuDir(0., 0., 0.), fDBName("SOLAR"), fRandomDir(false) {
  fElectron = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  fNue = G4ParticleTable::GetParticleTable()->FindParticle("nu_e");
  fNumu = G4ParticleTable::GetParticleTable()->FindParticle("nu_mu");
  fElectronMass = fElectron->GetPDGMass();
  fCCgen = new CCgen();
}

VertexGen_CC::~VertexGen_CC() {
  if (fCCgen) delete fCCgen;
}

void VertexGen_CC::GeneratePrimaryVertex(G4Event *argEvent, G4ThreeVector &dx, G4double dt) {
  //
  // Build the primary vertex with the position and time
  //
  G4PrimaryVertex *vertex = new G4PrimaryVertex(dx, dt);

  //
  // Build the incoming neutrino direction.
  // If this generator was not called from a higher level generator it is
  // possible that the neutrino direction is not set
  //
  if (fNuDir.mag2() == 0.0 || fRandomDir) {
    fRandomDir = true;
    // Pick isotropic direction
    double theta = acos(2.0 * G4UniformRand() - 1.0);
    double phi = 2.0 * G4UniformRand() * CLHEP::pi;
    fNuDir.setRThetaPhi(1.0, theta, phi);
  }

  // Generate charged current interaction using CCgen.
  // NB: Updated to keep track of the neutrino as well as the electron
  // For the moment the CCgen does not full mom_nu so it is empty
  //

  CLHEP::HepLorentzVector mom_electron, mom_nu;
  double e_nucleus = 0;
  fCCgen->GenerateEvent(fNuDir, mom_nu, mom_electron, e_nucleus);

  // -- Create particle at vertex
  // FIXME: Should we keep track of the outgoing neutrino as well?
  //       If so CCgen needs to be updated to pass the new neutrino direction.

  G4PrimaryParticle *electron_particle = new G4PrimaryParticle(fElectron,           // particle code
                                                               mom_electron.px(),   // x component of momentum
                                                               mom_electron.py(),   // y component of momentum
                                                               mom_electron.pz());  // z component of momentum
  electron_particle->SetMass(fElectronMass);  // This seems to help in VertexGen_IBD
  vertex->SetPrimary(electron_particle);

  // Add the Be7 nucleus leftover (at rest)
  // G4ParticleDefinition* be7_ion =
  // G4IonTable::GetIonTable()->FindIon(4,7,e_nucleus); G4PrimaryParticle*
  // be7_part = new G4PrimaryParticle(be7_ion, 0, 0, 0); //FIXME Do I need to
  // add the energy here also? -> Write a quick excitation generator
  // vertex->SetPrimary(be7_part);

  // Add the incoming neutrino as the primary
  G4PrimaryParticle *neutrinoparent;
  if (this->GetNuFlavor() == "nue") {
    neutrinoparent = new G4PrimaryParticle(fNue, mom_nu.px(), mom_nu.py(), mom_nu.pz());
  } else if (this->GetNuFlavor() == "numu") {
    neutrinoparent = new G4PrimaryParticle(fNumu, mom_nu.px(), mom_nu.py(), mom_nu.pz());
  } else {
    return;
  }

  // We DON'T Add this one to the vertex as we don't want to propagate it but
  // alongside so that the information is present for extraction in Gsim
  PrimaryVertexInformation *vertinfo = new PrimaryVertexInformation();
  vertinfo->AddNewParentParticle(neutrinoparent);

  vertex->SetUserInformation(vertinfo);
  argEvent->AddPrimaryVertex(vertex);
}

void VertexGen_CC::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);  // from GLG4StringUtil
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this VertexGen_CC:" << newline << " \"" << GetState() << "\"" << newline << newline;
    info << "Format of argument to VertexGen_CC::SetState: " << newline
         << " \"nu_dir_x nu_dir_y nu_dir_z [db_name:][db_flux:nu_flavor]\"\n"
         << " where fNuDir is the initial direction of the incoming neutrino.\n"
         << " Does not have to be normalized.  Set to \"0. 0. 0.\" for "
         << "isotropic\n"
         << " neutrino direction." << newline;
    return;
  }

  std::istringstream is(newValues.c_str());
  double x, y, z;
  std::string rest;
  is >> x >> y >> z >> rest;
  if (is.fail()) {
    info << "VertexGen_CC : Failed to extract state from input string." << newline;
    return;
  }

  // We take care of normalising the input direction here
  if (x == 0. && y == 0. && z == 0.)
    fNuDir.set(0., 0., 0.);
  else
    fNuDir = G4ThreeVector(x, y, z).unit();

  // Now check that everything else is in "rest"
  if (rest.length() == 0) return;

  std::string parseparams = ":";
  rest = strip(rest, parseparams);
  std::vector<std::string> params = util_split(rest, parseparams);
  switch (params.size()) {
    case 3:
      // First entry is the database name
      this->SetDBName(params[0]);
      this->SetFlux(params[1]);
      this->SetNuFlavor(params[2]);
      break;
    case 2:
      this->SetFlux(params[0]);
      this->SetNuFlavor(params[1]);
      break;
    case 1:
      this->SetDBName(params[0]);
    default:
      info << "VertexGen_CC : Detected only " << params.size() << " neutrino state terms (1,2, or 3 expected)."
           << newline;
      return;
  }
}

G4String VertexGen_CC::GetState() {
  std::ostringstream os;

  os << fNuDir.x() << "\t" << fNuDir.y() << "\t" << fNuDir.z() << std::ends;

  G4String rv(os.str());
  return rv;
}

void VertexGen_CC::SetFlux(const G4String flux) {
  if (fFlux == flux) return;
  fFlux = flux;
  fCCgen->SetNuType(fFlux);
}

void VertexGen_CC::SetNuFlavor(const G4String flavor) {
  if (fNuFlavor == flavor) return;
  fNuFlavor = flavor;
  fCCgen->SetNuFlavor(fNuFlavor);
}

void VertexGen_CC::SetDBName(const G4String name) {
  if (fDBName == name) return;
  fDBName = name;
  fCCgen->SetDBName(fDBName);
}

}  // namespace RAT
