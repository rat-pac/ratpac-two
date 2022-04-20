#ifndef __RAT__
#define __RAT__
#include <RAT/AnyParse.hh>
#include <TStopwatch.h>
#include <RAT/DB.hh>
#include <RAT/DBMessenger.hh>

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
  DB* rdb;
  DBMessenger* rdb_messenger;
public:
  Rat(AnyParse*, int, char**);
  ~Rat();
  void Begin();
  void Report();
};

} // namespace RAT

#endif
