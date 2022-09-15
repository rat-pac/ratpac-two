#include "RAT/GLG4TimeGen.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <Randomize.hh>

#include "RAT/GLG4StringUtil.hh"

void GLG4TimeGen_Uniform::SetState(G4String state) {
  state = util_strip_default(state);
  if (state.length() == 0) {
    // print help and current state
    std::cout << "Current state of this GLG4TimeGen:\n"
              << " \"" << GetState() << "\"\n"
              << std::endl;
    std::cout << "Format of argument to GLG4TimeGen::SetState: \n"
                 " \"rate\"\n"
                 " where rate is in 1/sec."
              << std::endl;
    return;
  }

  rate = util_to_double(state) / CLHEP::s;
}

G4String GLG4TimeGen_Uniform::GetState() const { return util_dformat("%lf", rate); }

// *****************************************************

double GLG4TimeGen_Poisson::GenerateEventTime(double offset) { return -log(1.0 - G4UniformRand()) / rate + offset; }
