#ifndef __RAT_NoiseProc__
#define __RAT_NoiseProc__

#include <RAT/Processor.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/PMTCharge.hh>
#include <RAT/PMTTime.hh>


namespace RAT {

class NoiseProc : public Processor {
public:
  NoiseProc();
  virtual ~NoiseProc() { };
  virtual Processor::Result DSEvent(DS::Root *ds);

  void AddNoiseHit(DS::MCPMT*, DS::PMTInfo*, double); 
  int GenerateNoiseInWindow( DS::MC*, double, 
      double, DS::PMTInfo*, std::map<int, int> );
  void UpdatePMTModels( DS::PMTInfo* );
  std::map<double, double> FindWindows(std::vector<double> &times, 
      double window);
  void SetD(std::string, double);
  void SetI(std::string param, int value);

protected:
  double fNoiseRate;
  double fLookback;
  double fLookforward;
  double fMaxTime;
  bool fNearHits;
  std::vector<RAT::PMTTime*> fPMTTime;
  std::vector<RAT::PMTCharge*> fPMTCharge;
};


} // namespace RAT

#endif
