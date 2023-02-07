#ifndef __RAT_PMTWaveformGenerator__
#define __RAT_PMTWaveformGenerator__

#include <TGraph.h>
#include <TObject.h>

#include <RAT/PMTWaveform.hh>
#include <RAT/DS/MCPMT.hh>
#include <vector>

namespace RAT {

class PMTWaveformGenerator {
 public:
  PMTWaveformGenerator();
  virtual ~PMTWaveformGenerator();

  virtual PMTWaveform GenerateWaveforms(DS::MCPMT* mcpmt, double triggerTime);

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
