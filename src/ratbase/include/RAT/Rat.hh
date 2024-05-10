#ifndef __RAT__
#define __RAT__
#include <TStopwatch.h>

#include <RAT/AnyParse.hh>
#include <RAT/DB.hh>
#include <RAT/DBMessenger.hh>
#include <RAT/RatMessenger.hh>
#include <set>

namespace RAT {

class Rat {
 protected:
  AnyParse *parser;
  long seed;
  std::string input_filename;
  std::string output_filename;
  std::string vector_filename;
  std::string chroma_filename;
  std::vector<std::string> python_processors;
  int run;
  bool vis;
  int argc;
  char **argv;
  TStopwatch runTime;
  DB *rdb;
  DBMessenger *rdb_messenger;
  RatMessenger *rat_messenger;

 public:
  inline static std::set<std::string> ratdb_directories = {};
  inline static std::set<std::string> model_directories = {};

  Rat(AnyParse *, int, char **);
  ~Rat();
  void Begin();
  void Report();
};

}  // namespace RAT

#endif
