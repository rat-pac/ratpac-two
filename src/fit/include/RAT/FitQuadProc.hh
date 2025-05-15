#ifndef __RAT_FitQuadProc__
#define __RAT_FitQuadProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>

namespace RAT {

class FitQuadProc : public Processor {
 public:
  FitQuadProc();
  virtual ~FitQuadProc() {}
  void BeginOfRun(DS::Run *run);
  virtual void SetI(std::string param, int value);
  virtual void SetD(std::string param, double value);
  Processor::Result Event(DS::Root *ds, DS::EV *ev);

 private:
  std::array<unsigned int, 4> ChoosePMTs(unsigned int nhits);
  std::vector<std::array<unsigned int, 4> > BuildTable(const unsigned int n);

  DS::Run *fRun;
  DS::PMTInfo *fPMTInfo;
  unsigned int fNumQuadPoints;
  unsigned int fMaxQuadPoints;
  unsigned int fTableCutOff;
  double fLightSpeed;
  double fMaxRadius;
  const std::array<unsigned int, 24> fNumPointsTbl = {0,    0,    0,    0,    1,    5,    15,   35,
                                                      70,   126,  210,  330,  495,  715,  1001, 1365,
                                                      1820, 2380, 3060, 3876, 4845, 5985, 7315, 8855};

 protected:
  FitterInputHandler inputHandler;
};

}  // namespace RAT

#endif
