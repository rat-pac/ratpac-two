#ifndef __RAT__
#define __RAT__
#include <TStopwatch.h>

#include <RAT/AnyParse.hh>
#include <RAT/DB.hh>
#include <RAT/DBMessenger.hh>
#include <RAT/Log.hh>
#include <RAT/ProducerBlock.hh>
#include <RAT/RatMessenger.hh>
#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace RAT {

/** An iterable, ordered list of RATDB search directories with explicit
 *  priority control.
 *
 *  Prepend()/Append() give a directory higher/lower priority than anything
 *  already present. If it's already in the list, it's moved rather than
 *  duplicated.
 *
 *  Forward iteration (begin()/end()) yields directories highest priority
 *  first. Reverse iteration (rbegin()/rend()) yields the same back-to-front,
 *  so loading each directory in reverse order and letting later loads
 *  overwrite earlier ones makes the highest-priority directory win (see
 *  DB::LoadDefaults()).
 *
 *  insert() is a legacy alias for downstream code that used to insert
 *  directly into the old std::set<std::string> ratdb_directories. A
 *  std::set has no priority order, so insert() treats the directory as
 *  highest priority (equivalent to Prepend()) and warns.
 **/

class RatdbDirectoryList {
 public:
  void Prepend(const std::string &dir) {
    // Delete if already exists
    fDirs.erase(std::remove(fDirs.begin(), fDirs.end(), dir), fDirs.end());
    fDirs.insert(fDirs.begin(), dir);
  }

  void Append(const std::string &dir) {
    fDirs.erase(std::remove(fDirs.begin(), fDirs.end(), dir), fDirs.end());
    fDirs.push_back(dir);
  }

  void insert(const std::string &dir) {
    warn << "RAT::Rat::ratdb_directories.insert() is deprecated: std::set has no priority order, so \"" << dir
         << "\" is being treated as highest priority (equivalent to Prepend()). Use Prepend()/Append() instead."
         << newline;
    Prepend(dir);
  }

  bool Contains(const std::string &dir) const { return std::find(fDirs.begin(), fDirs.end(), dir) != fDirs.end(); }

  std::vector<std::string>::const_iterator begin() const { return fDirs.begin(); }
  std::vector<std::string>::const_iterator end() const { return fDirs.end(); }
  std::vector<std::string>::const_reverse_iterator rbegin() const { return fDirs.rbegin(); }
  std::vector<std::string>::const_reverse_iterator rend() const { return fDirs.rend(); }

 private:
  std::vector<std::string> fDirs;
};

class Rat {
 protected:
  AnyParse *parser;
  long seed;
  std::string input_filename;
  std::string output_filename;
  std::string vector_filename;
  std::vector<std::string> python_processors;
  int run;
  bool vis;
  int argc;
  char **argv;
  TStopwatch runTime;
  DB *rdb;
  DBMessenger *rdb_messenger;
  RatMessenger *rat_messenger;
  ProducerBlock prodBlock;

 public:
  // Priority-ordered RATDB search directories, highest
  // priority first. Use Prepend()/Append() to add to this.
  inline static RatdbDirectoryList ratdb_directories = {};
  inline static std::set<std::string> model_directories = {};

  Rat(AnyParse *parser, int argc, char **argv) : parser(parser), argc(argc), argv(argv){};
  ~Rat();
  virtual void Configure();
  void Begin();
  void Report();
};

}  // namespace RAT

#endif
