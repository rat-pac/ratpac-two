// RAT::AmBeGen
// 10-Jan-2006 WGS

// See comments in RATAmBeGen.hh

#include <CLHEP/Vector/LorentzVector.h>
#include <CLHEP/Vector/ThreeVector.h>

#include <G4Event.hh>
#include <G4Gamma.hh>
#include <G4Neutron.hh>
#include <G4ParticleDefinition.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4ThreeVector.hh>
#include <G4UnitsTable.hh>
#include <RAT/AmBeGen.hh>
#include <RAT/AmBeSource.hh>
#include <RAT/Factory.hh>
#include <RAT/GLG4PosGen.hh>
#include <RAT/GLG4StringUtil.hh>
#include <RAT/GLG4TimeGen.hh>
#include <RAT/Log.hh>
#include <cstring>

#undef DEBUG

namespace RAT {

AmBeGen::AmBeGen() : stateStr(""), posGen(0) {
  // As in the combo generator, use a default time generator if the
  // user does not supply one.
  timeGen = new GLG4TimeGen_Poisson();

  // Initialize the decay particles.
  neutron = G4Neutron::Neutron();
  gamma = G4Gamma::Gamma();
}

AmBeGen::~AmBeGen() {
  delete timeGen;
  delete posGen;
}

void AmBeGen::GenerateEvent(G4Event *event) {
  // Initialize a new AmBe reaction.
  AmBeSource ambeSource;

  // Generate the position of the reation.  Note that, for now, we
  // don't change the position of the reaction (assumed to be localized).
  G4ThreeVector position;
  posGen->GeneratePosition(position);

  // Number of neutrons and prompt photons produced in the fission.
  int numberNeutrons = ambeSource.GetNumNeutron();
  int numberGammas = ambeSource.GetNumGamma();

#ifdef DEBUG
  debug << "RAT::AmBeGen::GenerateEvent: " << numberNeutrons << " neutrons, " << numberGammas << " photons" << newline;
#endif

  // For each neutron...
  for (int i = 0; i < numberNeutrons; i++) {
    // Get the particle's information (momentum and time)
    CLHEP::HepLorentzVector p = ambeSource.GetAmBeNeutronMomentum(i);
    double t = ambeSource.GetAmBeNeutronTime(i);

    // Adjust the time from the t0 of the event.
    G4double time = NextTime() + t;

    // generate a vertex with a primary particle
    G4PrimaryVertex *vertex = new G4PrimaryVertex(position, time);
    G4PrimaryParticle *particle = new G4PrimaryParticle(neutron, p.px(), p.py(), p.pz());
    particle->SetMass(neutron->GetPDGMass());  // Apparently this is useful in IBD code.
    vertex->SetPrimary(particle);
    event->AddPrimaryVertex(vertex);

#ifdef DEBUG
    debug << "RAT::AmBeGen::GenerateEvent: "
          << "Neutron " << i << " of " << numberNeutrons << "    time=" << G4BestUnit(time, "Time")
          << ", position=" << G4BestUnit(position, "Length") << ", momentum=" << G4BestUnit(p, "Energy") << newline;
#endif

  }  // for each neutron

  // For each prompt photon...
  for (int i = 0; i < numberGammas; i++) {
    // Get the particle's information (momentum and time)
    CLHEP::HepLorentzVector p = ambeSource.GetAmBeGammaMomentum(i);
    double t = ambeSource.GetAmBeGammaTime(i);

    // Adjust the time from the t0 of the event.
    G4double time = NextTime() + t;

    // generate a vertex with a primary particle
    G4PrimaryVertex *vertex = new G4PrimaryVertex(position, time);
    G4PrimaryParticle *particle = new G4PrimaryParticle(gamma, p.px(), p.py(), p.pz());
    particle->SetMass(gamma->GetPDGMass());  // Who knows?  Let's do this the
                                             // same way as the others.
    vertex->SetPrimary(particle);
    event->AddPrimaryVertex(vertex);

#ifdef DEBUG
    debug << "RAT::AmBeGen::GenerateEvent: "
          << "Gamma " << i << " of " << numberGammas << "    time=" << G4BestUnit(time, "Time")
          << ", position=" << G4BestUnit(position, "Length") << ", momentum=" << G4BestUnit(p, "Energy") << newline;
#endif

  }  // for each prompt photon
}

void AmBeGen::ResetTime(double offset) {
  double eventTime = timeGen->GenerateEventTime();
  nextTime = eventTime + offset;
#ifdef DEBUG
  debug << "RAT::AmBeGen::ResetTime:"
        << " eventTime=" << G4BestUnit(eventTime, "Time") << ", offset=" << G4BestUnit(offset, "Time")
        << ", nextTime=" << G4BestUnit(nextTime, "Time") << newline;
#endif
}

void AmBeGen::SetState(G4String state) {
#ifdef DEBUG
  debug << "RAT::AmBeGen::SetState called with state='" << state << "'" << newline;
#endif

  // Break the argument to the this generator into sub-std::strings
  // separated by ":".
  state = util_strip_default(state);
  std::vector<std::string> parts = util_split(state, ":");
  size_t nArgs = parts.size();

#ifdef DEBUG
  debug << "RAT::AmBeGen::SetState: nArgs=" << nArgs << newline;
#endif

  try {
    if (nArgs >= 2) {
      // The last argument is an optional time generator
      delete timeGen;
      timeGen = 0;  // In case of exception in next line
      timeGen = GlobalFactory<GLG4TimeGen>::New(parts[1]);
    }

    if (nArgs >= 1) {
      // This argument is a position generator.
      delete posGen;
      posGen = 0;
      posGen = GlobalFactory<GLG4PosGen>::New(parts[0]);
    } else {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("AmBeGen syntax error: '" + state + "' does not have a position generator").c_str());
    }

    stateStr = state;  // Save for later call to GetState()
  } catch (FactoryUnknownID &unknown) {
    warn << "Unknown generator \"" << unknown.id << "\"" << newline;
  }
}

G4String AmBeGen::GetState() const { return stateStr; }

void AmBeGen::SetTimeState(G4String state) {
  if (timeGen)
    timeGen->SetState(state);
  else
    warn << "AmBeGen error: Cannot set time state, no time generator selected" << newline;
}

G4String AmBeGen::GetTimeState() const {
  if (timeGen)
    return timeGen->GetState();
  else
    return G4String("AmBeGen error: no time generator selected");
}

void AmBeGen::SetPosState(G4String state) {
  if (posGen)
    posGen->SetState(state);
  else
    warn << "AmBeGen error: Cannot set position state, no position generator "
            "selected"
         << newline;
}

G4String AmBeGen::GetPosState() const {
  if (posGen)
    return posGen->GetState();
  else
    return G4String("AmBeGen error: no position generator selected");
}

}  // namespace RAT
