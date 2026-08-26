#ifndef __RAT__
#define __RAT__
#include <TStopwatch.h>

#include <RAT/AnyParse.hh>
#include <RAT/DB.hh>
#include <RAT/DBMessenger.hh>
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
 *  duplicated. Both also insert into the legacy Rat::ratdb_directories set so
 *  old code that inserts into or iterates that set directly still works and
 *  sees everything.
 *
 *  Forward iteration (begin()/end()) yields Prepended/Appended directories in
 *  priority order, then whatever's left in ratdb_directories. Reverse
 *  iteration (rbegin()/rend()) yields the same back-to-front, so loading each
 *  directory in reverse order and letting later loads overwrite earlier ones
 *  makes the highest-priority directory win (see DB::LoadDefaults()). Every
 *  begin()/rbegin() call re-pulls ratdb_directories' current contents into
 *  this combined view; don't hold an iterator across a fresh begin()/rbegin()
 *  call, since it rewrites the storage that iterator points into. Normal
 *  iteration (which calls begin()/rbegin() once, then end()/rend() once) is
 *  unaffected.
 **/

class RatdbDirectoryList {
 public:
  void Prepend(const std::string &dir);
  void Append(const std::string &dir);
  bool Contains(const std::string &dir) const { return std::find(dirs_.begin(), dirs_.end(), dir) != dirs_.end(); }

  std::vector<std::string>::const_iterator begin() const;
  std::vector<std::string>::const_iterator end() const { return combined_.end(); }
  std::vector<std::string>::const_reverse_iterator rbegin() const;
  std::vector<std::string>::const_reverse_iterator rend() const { return combined_.rend(); }

 private:
  void RebuildCombined() const;

  std::vector<std::string> dirs_;
  mutable std::vector<std::string> combined_;
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
  // Priority-ordered RATDB search directories (e.g. RATDB_EXTRA_PATH), highest
  // priority first. Use Prepend()/Append() to add to this rather than
  // inserting into ratdb_directories, so priority order is preserved.
  inline static RatdbDirectoryList ratdb_search_path = {};
  // Retained for backwards compatibility with code that inserts into it directly.
  inline static std::set<std::string> ratdb_directories = {};
  inline static std::set<std::string> model_directories = {};

  Rat(AnyParse *parser, int argc, char **argv) : parser(parser), argc(argc), argv(argv){};
  ~Rat();
  virtual void Configure();
  void Begin();
  void Report();
};

inline void RatdbDirectoryList::Prepend(const std::string &dir) {
  // Delete if already exists
  dirs_.erase(std::remove(dirs_.begin(), dirs_.end(), dir), dirs_.end());
  dirs_.insert(dirs_.begin(), dir);
  Rat::ratdb_directories.insert(dir);
}

inline void RatdbDirectoryList::Append(const std::string &dir) {
  dirs_.erase(std::remove(dirs_.begin(), dirs_.end(), dir), dirs_.end());
  dirs_.push_back(dir);
  Rat::ratdb_directories.insert(dir);
}

inline void RatdbDirectoryList::RebuildCombined() const {
  combined_ = dirs_;
  for (const auto &dir : Rat::ratdb_directories) {
    if (!Contains(dir)) combined_.push_back(dir);
  }
}

inline std::vector<std::string>::const_iterator RatdbDirectoryList::begin() const {
  RebuildCombined();
  return combined_.begin();
}

inline std::vector<std::string>::const_reverse_iterator RatdbDirectoryList::rbegin() const {
  RebuildCombined();
  return combined_.rbegin();
}

}  // namespace RAT

#endif
