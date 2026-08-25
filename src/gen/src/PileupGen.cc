#include "RAT/PileupGen.hh"

#include <G4Event.hh>
#include <G4ThreeVector.hh>
#include <RAT/Factory.hh>
#include <RAT/Log.hh>
#include <RAT/PileupMessenger.hh>
#include <Randomize.hh>
#include <algorithm>
#include <cmath>

#include "RAT/GLG4PosGen.hh"
#include "RAT/GLG4StringUtil.hh"
#include "RAT/GLG4TimeGen.hh"
#include "RAT/GLG4VertexGen.hh"

namespace RAT {

void PileupDist::SetState(G4String state) {
  state = util_strip_default(state);

  if (state.length() == 0) {
    // print help and current state
    info << "Current state of this PileupDist: \"" << GetState() << "\"" << newline << newline;
    info << "Format of argument to PileupDist::SetState: " << newline
         << " \"fixed:value\" | \"uniform:min:max\" | \"poisson:mean\"" << newline;
    return;
  }

  std::vector<std::string> parts = util_split(state, ":");

  if (parts[0] == "fixed") {
    if (parts.size() != 2) {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("PileupDist syntax error, expected \"fixed:value\": " + state).c_str());
      return;  // G4Exception(FatalException) aborts; return anyway so
               // parts[1] below is never read out of bounds.
    }
    type = kPileupFixed;
    fixedValue = util_to_double(parts[1]);
  } else if (parts[0] == "uniform") {
    if (parts.size() != 3) {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("PileupDist syntax error, expected \"uniform:min:max\": " + state).c_str());
      return;  // see above
    }
    type = kPileupUniform;
    uniformMin = util_to_double(parts[1]);
    uniformMax = util_to_double(parts[2]);
  } else if (parts[0] == "poisson") {
    if (parts.size() != 2) {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("PileupDist syntax error, expected \"poisson:mean\": " + state).c_str());
      return;  // see above
    }
    type = kPileupPoisson;
    poissonMean = util_to_double(parts[1]);
    if (poissonMean <= 0.0) {
      G4Exception(__FILE__, "Invalid Parameter", FatalException,
                  ("PileupDist poisson mean must be > 0: " + state).c_str());
    }
  } else {
    G4Exception(__FILE__, "Invalid Parameter", FatalException,
                ("PileupDist unknown distribution type: " + parts[0]).c_str());
  }
}

G4String PileupDist::GetState() const {
  switch (type) {
    case kPileupFixed:
      return util_dformat("fixed:%lf", fixedValue);
    case kPileupUniform:
      return util_dformat("uniform:%lf:%lf", uniformMin, uniformMax);
    case kPileupPoisson:
      return util_dformat("poisson:%lf", poissonMean);
  }
  return "";
}

std::vector<double> PileupDist::GenerateTimes(uint64_t n, double startTime) const {
  std::vector<double> times(n);
  switch (type) {
    case kPileupFixed:
      for (uint64_t i = 0; i < n; i++) times[i] = startTime + i * fixedValue;
      break;
    case kPileupUniform:
      for (uint64_t i = 0; i < n; i++) times[i] = startTime + CLHEP::RandFlat::shoot(uniformMin, uniformMax);
      std::sort(times.begin(), times.end());
      break;
    case kPileupPoisson:
      for (uint64_t i = 0; i < n; i++) times[i] = startTime + CLHEP::RandExponential::shoot(poissonMean);
      std::sort(times.begin(), times.end());
      break;
  }
  return times;
}

uint64_t PileupDist::SampleMultiplicity() const {
  switch (type) {
    case kPileupFixed:
      return static_cast<uint64_t>(std::round(fixedValue));
    case kPileupUniform:
      return static_cast<uint64_t>(
          CLHEP::RandFlat::shootInt(static_cast<long>(uniformMin), static_cast<long>(uniformMax) + 1));
    case kPileupPoisson: {
      // Draw from zero truncated Poisson distribution
      uint64_t n = 0;
      while (n < 1) {
        n = static_cast<uint64_t>(CLHEP::RandPoisson::shoot(poissonMean));
      }
      return n;
    }
  }
  return 0;
}

bool PileupDist::MultiplicityConfigIsValid() const {
  switch (type) {
    case kPileupFixed:
      return std::round(fixedValue) >= 1.0;
    case kPileupUniform:
      return std::floor(uniformMin) >= 1.0;
    case kPileupPoisson:
      // SampleMultiplicity() rejects zero samples, so this is always safe.
      return true;
  }
  return true;
}

PileupGen::PileupGen() : stateStr(""), vertexGen(nullptr), posGen(nullptr) {
  timeGen = new GLG4TimeGen_Poisson();
  multiplicityDist.SetState("fixed:1");
  spacingDist.SetState("fixed:0");
  messenger = std::make_unique<PileupMessenger>(this);
}

PileupGen::~PileupGen() {
  delete timeGen;
  delete vertexGen;
  delete posGen;
}

void PileupGen::GenerateEvent(G4Event *event) {
  uint64_t n = multiplicityDist.SampleMultiplicity();
  if (n < 1) {
    G4Exception(__FILE__, "Invalid Multiplicity", FatalException,
                "PileupGen sampled zero vertices for this event; every pileup event must "
                "contain at least one vertex. Adjust the multiplicity configuration.");
    return;
  }

  std::vector<double> times = spacingDist.GenerateTimes(n, NextTime());
  for (uint64_t i = 0; i < n; i++) {
    G4ThreeVector pos;
    posGen->GeneratePosition(pos);
    vertexGen->GeneratePrimaryVertex(event, pos, times[i]);
  }
}

void PileupGen::ResetTime(double offset) { nextTime = timeGen->GenerateEventTime() + offset; }

void PileupGen::SetState(G4String state) {
  state = util_strip_default(state);

  std::vector<std::string> parts = util_split(state, ":");

  try {
    switch (parts.size()) {
      case 3:
        // last is optional time generator
        delete timeGen;
        timeGen = nullptr;  // In case of exception in next line
        timeGen = RAT::GlobalFactory<GLG4TimeGen>::New(parts[2]);
        [[fallthrough]];
      case 2:
        delete vertexGen;
        vertexGen = nullptr;
        vertexGen = RAT::GlobalFactory<GLG4VertexGen>::New(parts[0]);
        delete posGen;
        posGen = nullptr;
        posGen = RAT::GlobalFactory<GLG4PosGen>::New(parts[1]);
        break;
      default:
        G4Exception(__FILE__, "Invalid Parameter", FatalException, ("Pileup generator syntax error: " + state).c_str());
        break;
    }

    stateStr = state;  // Save for later call to GetState()
  } catch (RAT::FactoryUnknownID &unknown) {
    warn << "Unknown generator \"" << unknown.id << "\"" << newline;
  }
}

G4String PileupGen::GetState() const { return stateStr; }

void PileupGen::SetTimeState(G4String state) {
  if (timeGen)
    timeGen->SetState(state);
  else
    warn << "PileupGen error: Cannot set time state, no time generator selected" << newline;
}

G4String PileupGen::GetTimeState() const {
  if (timeGen)
    return timeGen->GetState();
  else
    return G4String("PileupGen error: no time generator selected");
}

void PileupGen::SetVertexState(G4String state) {
  if (vertexGen)
    vertexGen->SetState(state);
  else
    warn << "PileupGen error: Cannot set vertex state, no vertex generator selected" << newline;
}

G4String PileupGen::GetVertexState() const {
  if (vertexGen)
    return vertexGen->GetState();
  else
    return G4String("PileupGen error: no vertex generator selected");
}

void PileupGen::SetPosState(G4String state) {
  if (posGen)
    posGen->SetState(state);
  else
    warn << "PileupGen error: Cannot set position state, no position generator selected" << newline;
}

G4String PileupGen::GetPosState() const {
  if (posGen)
    return posGen->GetState();
  else
    return G4String("PileupGen error: no pos generator selected");
}

void PileupGen::SetMultiplicityState(G4String state) {
  multiplicityDist.SetState(state);
  if (!multiplicityDist.MultiplicityConfigIsValid()) {
    G4Exception(__FILE__, "Invalid Parameter", FatalException,
                ("PileupGen multiplicity configuration can produce zero vertices per event: \"" + state +
                 "\". Every pileup event must contain at least one vertex.")
                    .c_str());
  }
}

G4String PileupGen::GetMultiplicityState() const { return multiplicityDist.GetState(); }

void PileupGen::SetSpacingState(G4String state) { spacingDist.SetState(state); }

G4String PileupGen::GetSpacingState() const { return spacingDist.GetState(); }

}  // namespace RAT
