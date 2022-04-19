#ifndef __RAT__
#define __RAT__
#include <RAT/AnyParse.hh>
#include <TStopwatch.h>

namespace RAT {

class Rat {
  AnyParse *parser;
  long seed;
  std::string input_filename;
  std::string output_filename;
  std::string vector_filename;
  std::vector<std::string> python_processors;
  int run;
  bool vis;
  int argc;
  char** argv;
  TStopwatch runTime;
public:
  Rat(AnyParse*, int, char**);
  void Begin();
  void Report();
};

} // namespace RAT

#endif
