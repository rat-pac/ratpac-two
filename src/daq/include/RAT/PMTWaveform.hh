#ifndef __RAT_PMTWaveform__
#define __RAT_PMTWaveform__

#include <TGraph.h>
#include <TObject.h>

#include <RAT/DB.hh>
#include <RAT/PMTPulse.hh>
#include <RAT/DS/MCPMT.hh>
#include <vector>

namespace RAT {

class PMTWaveform {
 public:
  PMTWaveform();
  virtual ~PMTWaveform();
  virtual double GetHeight(double time);

  virtual PMTWaveform GenerateWaveforms(DS::MCPMT* mcpmt, double triggerTime);

  std::vector<PMTPulse *> fPulse;

  DBLinkPtr lpulse;
  double fPMTPulseMin;
  double fPMTPulseOffset;
  double fPMTPulseTimeOffset;
  double fPMTPulseWidth;
  double fPMTPulseMean;
  double fTerminationOhms;
};

}  // namespace RAT

#endif
