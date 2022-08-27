#ifndef __RAT_AfterPulseProc__
#define __RAT_AfterPulseProc__

#include <RAT/Processor.hh>
#include <RAT/DS/PMTInfo.hh>

namespace RAT {

class AfterPulseProc : public Processor {
public:
  AfterPulseProc();
  virtual ~AfterPulseProc() { };
  virtual Processor::Result DSEvent(DS::Root *ds);

  void GenerateAfterPulse(DS::MC*, DS::PMTInfo*, uint64_t);  
  double CalculateAfterPulseTime(double, std::vector<double>, std::vector<double>);
  double PickFrontEnd(double, std::vector<double>, std::vector<double>, double);
  void DetectAfterPulse(DS::MC*, RAT::DS::PMTInfo*, uint64_t, std::vector<double>, std::vector<double>, double);

  void SetD(std::string, double);
  void SetI(std::string param, int value);

 

protected:
  std::string fDAQ;
  int fAPFlag;
  double fDefAPFraction;
  double fTriggerWindow;
  double fDefCableDelay;
  int fPMTCount;

  std::vector<double> fDefAPTime;
  std::vector<double> fDefAPProb;

  std::vector<double> fDefPMTTime;
  std::vector<double> fDefPMTProb;

  std::vector<std::vector<uint64_t> > fAfterPulseTime;
  std::vector <std::vector<uint64_t> > fFrontEndTime;
  std::vector<double> fPMTTime;
  std::vector<double> fPMTProb;
  std::vector <size_t> countAP;

};


} // namespace RAT

#endif
