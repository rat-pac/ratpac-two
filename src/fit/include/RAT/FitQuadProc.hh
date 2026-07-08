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
  virtual void SetS(std::string param, std::string value);
  virtual void SetI(std::string param, int value);
  virtual void SetD(std::string param, double value);
  Processor::Result Event(DS::Root *ds, DS::EV *ev);

 private:
  std::array<unsigned int, 4> ChoosePMTs(unsigned int nhits);
  std::vector<std::array<unsigned int, 4> > BuildTable(const unsigned int n);

  std::vector<int> fPMTtype;   // Types of PMT to use in reconstruction.  If empty, uses all PMT types.
  std::string fFitLabel = "";  // Label for the fit result.  User can specify.
  DS::Run *fRun;
  DS::PMTInfo *fPMTInfo;
  unsigned int fNumQuadPoints;
  unsigned int fMaxQuadPoints;
  unsigned int fTableCutOff;
  double fLightSpeed;  // [mm/ns].  Defaults to value in FIT_COMMON table.
  double fMaxRadius;   // [mm].  Defaults to value in FIT_QUAD table.
  double fMaxX;        // [mm].  Optional Cartesian alternative to fMaxRadius.
  double fMaxY;
  double fMaxZ;
  double fMaxHitTime = -9999;  // [ns].  Optional hit time limits - ineffective when fMaxHitTime <= fMinHitTime.
  double fMinHitTime = 9999;   // [ns].
  bool fSetMaxHitTime = false;
  bool fSetMinHitTime = false;
  const std::array<unsigned int, 24> fNumPointsTbl = {0,    0,    0,    0,    1,    5,    15,   35,
                                                      70,   126,  210,  330,  495,  715,  1001, 1365,
                                                      1820, 2380, 3060, 3876, 4845, 5985, 7315, 8855};

 protected:
  FitterInputHandler inputHandler;
};

}  // namespace RAT

#endif
