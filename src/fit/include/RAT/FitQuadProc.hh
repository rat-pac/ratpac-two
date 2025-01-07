#ifndef __RAT_FitQuadProc__
#define __RAT_FitQuadProc__

#include <RAT/Processor.hh>

namespace RAT {

class FitQuadProc : public Processor {
 public:
  FitQuadProc();
  virtual ~FitQuadProc() {}
  void BeginOfRun(DS::Run *run);
  Processor::Result Event(DS::Root *ds, DS::EV *ev);

 private:
  std::array<uint, 4> ChoosePMTs(uint nhits);
  std::vector<std::array<uint, 4> > BuildTable(const unsigned int n);

  DS::Run *fRun;
  DS::PMTInfo *fPMTInfo;
  uint fNumQuadPoints;
  uint fMaxQuadPoints;
  uint fTableCutOff;
  float fLightSpeed;
  std::vector<uint> fNumPointsTbl = {0,   0,   0,   0,   1,    5,    15,   35,   70,   126,
                                     210, 330, 495, 715, 1001, 1365, 1820, 2380, 3060, 3876};
};

}  // namespace RAT

#endif
