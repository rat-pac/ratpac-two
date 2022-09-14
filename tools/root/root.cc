#include <cstdlib>
#include <iostream>
#include <string>

#include "TROOT.h"
#include "TRint.h"

/* Shut up the silly RooFit banner */
Int_t doBanner() { return 0; }

int main(int argc, char *argv[]) {
  TRint *app = new TRint("ROOT for RAT", &argc, argv);
  std::string ratroot = getenv("RATROOT");
  std::string initmacro = ratroot + std::string("/src/rootinit.C");
  gROOT->ProcessLine((std::string(".x ") + initmacro).c_str());
  std::cout << "RAT: Libraries loaded." << std::endl;
  app->Run();
  return 0;
}
