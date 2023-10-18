#ifndef __RAT_AfterPulseProc__
#define __RAT_AfterPulseProc__

#include <RAT/DS/PMTInfo.hh>
#include <RAT/PMTCharge.hh>
#include <RAT/PMTTime.hh>
#include <RAT/Processor.hh>
#include <map>

namespace RAT {

class AfterPulseProc : public Processor {
 public:
  AfterPulseProc();
  virtual ~AfterPulseProc(){};
  virtual Processor::Result DSEvent(DS::Root* ds);
  void BeginOfRun(DS::Run* run);
  void UpdatePMTModels(DS::PMTInfo* pmtinfo);

  void GenerateAfterPulses(DS::MC* mc, DS::PMTInfo* pmtinfo, uint64_t eventTime);
  double CalculateAfterPulseTime(double apFraction, std::vector<double> apTime, std::vector<double> apProb);
  void DetectAfterPulses(DS::MC* mc, DS::PMTInfo* pmtinfo, uint64_t eventTime);

  void SetD(std::string param, double value);
  void SetI(std::string param, int value);

 protected:
  std::string fDAQ;
  int fAPFlag;
  double fTriggerWindow;

  double fDefaultAPFraction;
  std::vector<double> fDefaultAPTime;
  std::vector<double> fDefaultAPProb;

  std::map<std::string, double> fModelAPFractionMap;
  std::map<std::string, std::vector<double>> fModelAPTimeMap;
  std::map<std::string, std::vector<double>> fModelAPProbMap;

  std::vector<RAT::PMTTime*> fPMTTime;
  std::vector<RAT::PMTCharge*> fPMTCharge;

  std::map<int, std::vector<uint64_t>> fAfterPulseTime;
};

}  // namespace RAT

#endif
