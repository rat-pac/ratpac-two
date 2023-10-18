#include "RAT/GLG4TimeGen.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <RAT/Log.hh>
#include <Randomize.hh>

#include "RAT/GLG4StringUtil.hh"

void GLG4TimeGen_Uniform::SetState(G4String state) {
  state = util_strip_default(state);
  if (state.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4TimeGen:" << newline << " \"" << GetState() << "\"" << newline << newline;
    RAT::info << "Format of argument to GLG4TimeGen::SetState: " << newline << " \"rate\"\n"
              << " where rate is in 1/sec." << newline;
    return;
  }

  rate = util_to_double(state) / CLHEP::s;
}

G4String GLG4TimeGen_Uniform::GetState() const { return util_dformat("%lf", rate); }

// *****************************************************

double GLG4TimeGen_Poisson::GenerateEventTime(double offset) { return -log(1.0 - G4UniformRand()) / rate + offset; }
