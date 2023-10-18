#ifndef __RAT_MuteGeant4__
#define __RAT_MuteGeant4__

#include <G4ios.hh>

namespace RAT {

class discard_streambuf : public std::streambuf {
 public:
  discard_streambuf(){};

  virtual int_type overflow(int_type c) { return c; };
};

discard_streambuf discard;
std::streambuf *g4cout_orig = G4cout.rdbuf();
std::streambuf *g4cerr_orig = G4cerr.rdbuf();

void mute_g4mute() {
  G4cout.rdbuf(&discard);
  G4cerr.rdbuf(&discard);
}

void mute_g4unmute() {
  G4cout.rdbuf(g4cout_orig);
  G4cerr.rdbuf(g4cerr_orig);
}

}  // namespace RAT

#endif
