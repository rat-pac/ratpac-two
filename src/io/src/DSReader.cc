#include <RAT/DSReader.hh>
#include <RAT/Log.hh>
#include <iostream>

namespace RAT {

#undef DEBUG

DSReader::DSReader(const char *filename) : T("T"), runT("runT") {
  T.Add(filename);

  next = 0;
  total = T.GetEntries();

#ifdef DEBUG
  debug << "DSReader::DSReader - "
        << "filename='" << filename << "', total=" << total << newline;
#endif

  ds = new DS::Root();
  T.SetBranchAddress("ds", &ds);
}

DSReader::~DSReader() { delete ds; }

void DSReader::Add(const char *filename) {
  T.Add(filename);
  runT.Add(filename);
  total = T.GetEntries();

#ifdef DEBUG
  debug << "DSReader::Add - "
        << "filename='" << filename << "', total=" << total << newline;
#endif
}

DS::Root *DSReader::NextEvent() {
  if (next < total) {
    T.GetEntry(next);
    next++;
    return ds;
  } else
    return 0;
}

}  // namespace RAT
