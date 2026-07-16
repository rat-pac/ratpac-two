#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include "RAT/Config.hh"

#include "TROOT.h"
#include "TRint.h"

/* Shut up the silly RooFit banner */
Int_t doBanner() { return 0; }

int main(int argc, char *argv[]) {
  TRint *app = new TRint("ROOT for RAT", &argc, argv);
  std::cout << "\n  ***************************************************\n"
            << "  *   T H I S   I S   R A T P A C 2 ' s   R O O T   *\n"
            << "  *                                                 *\n"
            << "  *     RAT Version: " << std::setw(31) << std::left << RAT::RATVERSION << "*\n"
            << "  *                                                 *\n"
            << "  ***************************************************\n\n";
  std::string ratroot = getenv("RATROOT");
  std::string initmacro = ratroot + std::string("/src/rootinit.C");
  gROOT->ProcessLine((std::string(".x ") + initmacro).c_str());
  std::cout << "RAT: Libraries loaded." << std::endl;
  app->Run();
  return 0;
}
