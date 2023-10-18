// fsutanto@umich.edu
// Apr 15, 2018
// almost copy pasted from CfGen.cc

#include <CLHEP/Vector/LorentzVector.h>

#include <G4Electron.hh>
#include <G4Event.hh>
#include <G4Gamma.hh>
#include <G4Neutron.hh>
#include <G4ParticleDefinition.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4ThreeVector.hh>
#include <G4UnitsTable.hh>
#include <RAT/DB.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4PosGen.hh>
#include <RAT/GLG4StringUtil.hh>
#include <RAT/GLG4TimeGen.hh>
#include <RAT/GdGen.hh>
#include <RAT/Log.hh>
#include <string>

#include "Randomize.hh"

#undef DEBUG

namespace RAT {

GdGen::GdGen() : stateStr(""), isotope(158), posGen(0) {
  // As in the combo generator, use a default time generator if the
  // user does not supply one.
  timeGen = new GLG4TimeGen_Poisson();

  // Initialize the decay particles.
  electron = G4Electron::Electron();
  gamma = G4Gamma::Gamma();
}

GdGen::~GdGen() {
  delete timeGen;
  delete posGen;
}

void GdGen::GenerateEvent(G4Event *event) {
  // Generate the position of the isotope.  Note that, for now, we
  // don't change the position of the isotope as it decays.
  G4ThreeVector position;
  posGen->GeneratePosition(position);

  // get the data from the database
  RAT::DBLinkPtr model = RAT::DB::Get()->GetLink("DICEBOX158GD", "158gd");

  // get number of secondaries
  theCdf = model->GetDArray("mul_cdf");
  theMul = model->GetIArray("mul");
  G4double myRand = G4UniformRand();
  G4int indexNow = 0;
  while (1) {
    if (theCdf[indexNow] > myRand) break;
    indexNow++;
  }
  int numSecondaries = theMul[indexNow];

  // get std::vector of energy for certain mutliplicity
  G4String tableErgName = "erg";
  tableErgName += std::to_string(numSecondaries);
  tableErgName += "_list";
  theErg = model->GetDArray(tableErgName);

  // get std::vector of particle type for certain mutliplicity
  G4String tableParName = "par";
  tableParName += std::to_string(numSecondaries);
  tableParName += "_list";
  thePar = model->GetIArray(tableParName);

  // Get number of secondaries
  G4int chosenIndex = floor(G4UniformRand() * thePar.size() / numSecondaries);

  // For each secondary
  for (int i = 0; i < numSecondaries; i++) {
    // propose type of particle
    // 0=photon, 1,2=electrons (K,L shells)
    G4int type = (thePar[numSecondaries * chosenIndex + i]);

    // propose the kinetic energy of new particle
    G4double erg = (theErg[numSecondaries * chosenIndex + i]) * CLHEP::MeV;

    // propose new random momentum
    G4double cost = 1. - 2. * G4UniformRand();
    G4double sint = sqrt(1. - cost * cost);

    G4double phi = 2. * CLHEP::pi * G4UniformRand();
    G4double sinp = sin(phi);
    G4double cosp = cos(phi);

    G4double px = sint * cosp;
    G4double py = sint * sinp;
    G4double pz = 0.0;
    if (G4UniformRand() > 0.5) {
      pz = cost;
    } else {
      pz = -cost;
    }

    // Adjust the time from the t0 of the event.
    double t = 0.0;
    G4double time = NextTime() + t;

    // generate a vertex with a primary particle
    G4PrimaryVertex *vertex = new G4PrimaryVertex(position, time);
    G4PrimaryParticle *particle;

    if (type == 0) {
      particle = new G4PrimaryParticle(gamma, px * erg, py * erg, pz * erg);
      particle->SetMass(gamma->GetPDGMass());
    } else {
      double restErg = electron->GetPDGMass() * CLHEP::MeV;
      erg = sqrt(erg * erg + erg * 2.0 * restErg);
      particle = new G4PrimaryParticle(electron, px * erg, py * erg, pz * erg);
      particle->SetMass(electron->GetPDGMass());
    }
    vertex->SetPrimary(particle);
    event->AddPrimaryVertex(vertex);
  }
}

void GdGen::ResetTime(double offset) {
  double eventTime = timeGen->GenerateEventTime();
  nextTime = eventTime + offset;
#ifdef DEBUG
  debug << "RAT::GdGen::ResetTime:"
        << " eventTime=" << G4BestUnit(eventTime, "Time") << ", offset=" << G4BestUnit(offset, "Time")
        << ", nextTime=" << G4BestUnit(nextTime, "Time") << newline;
#endif
}

void GdGen::SetState(G4String state) {
#ifdef DEBUG
  debug << "RAT::GdGen::SetState called with state='" << state << "'" << newline;
#endif

  // Break the argument to the this generator into sub-std::strings
  // separated by ":".
  state = util_strip_default(state);
  std::vector<std::string> parts = util_split(state, ":");
  size_t nArgs = parts.size();

#ifdef DEBUG
  debug << "RAT::GdGen::SetState: nArgs=" << nArgs << newline;
#endif

  try {
    if (nArgs >= 3) {
      // The last argument is an optional time generator
      delete timeGen;
      timeGen = 0;  // In case of exception in next line
      timeGen = GlobalFactory<GLG4TimeGen>::New(parts[2]);
    }

    if (nArgs >= 2) {
      // The first argument is the Californium isotope.  At
      // present, only Cf252 is supported.
      isotope = util_to_int(parts[0]);

      if (isotope != 158) {
        warn << "RAT::GdGen::SetState: Only gd 158 is supported" << newline;
      }

      // The second argument is a position generator.
      delete posGen;
      posGen = 0;
      posGen = GlobalFactory<GLG4PosGen>::New(parts[1]);
    } else {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("GdGen syntax error: '" + state + "' does not have a position generator").c_str());
    }

    stateStr = state;  // Save for later call to GetState()
  } catch (FactoryUnknownID &unknown) {
    warn << "Unknown generator \"" << unknown.id << "\"" << newline;
  }
}

G4String GdGen::GetState() const { return stateStr; }

void GdGen::SetTimeState(G4String state) {
  if (timeGen)
    timeGen->SetState(state);
  else
    warn << "GdGen error: Cannot set time state, no time generator selected" << newline;
}

G4String GdGen::GetTimeState() const {
  if (timeGen)
    return timeGen->GetState();
  else
    return G4String("GdGen error: no time generator selected");
}

void GdGen::SetPosState(G4String state) {
  if (posGen)
    posGen->SetState(state);
  else
    warn << "GdGen error: Cannot set position state, no position generator "
            "selected"
         << newline;
}

G4String GdGen::GetPosState() const {
  if (posGen)
    return posGen->GetState();
  else
    return G4String("GdGen error: no position generator selected");
}

}  // namespace RAT
