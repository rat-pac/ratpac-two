#ifndef __RAT_NoiseProc__
#define __RAT_NoiseProc__

#include <RAT/DS/PMTInfo.hh>
#include <RAT/PMTCharge.hh>
#include <RAT/PMTTime.hh>
#include <RAT/Processor.hh>
#include <map>

namespace RAT {

class NoiseProc : public Processor {
public:
  NoiseProc();
  virtual ~NoiseProc(){};
  virtual Processor::Result DSEvent(DS::Root *ds);

  void AddNoiseHit(DS::MCPMT *, DS::PMTInfo *, double);
  int GenerateNoiseInWindow(DS::MC *, double, double, DS::PMTInfo *,
                            std::map<int, int>);
  void UpdatePMTModels(DS::PMTInfo *);
  std::map<double, double> FindWindows(std::vector<double> &times,
                                       double window);
  void SetD(std::string, double);
  void SetI(std::string param, int value);

protected:
  double fDefaultNoiseRate;
  double fLookback;
  double fLookforward;
  double fMaxTime;
  bool fNearHits;
  bool fNoiseFlag;
  std::vector<RAT::PMTTime *> fPMTTime;
  std::vector<RAT::PMTCharge *> fPMTCharge;
  std::map<std::string, double> fModelNoiseMap;
};

} // namespace RAT

#endif
