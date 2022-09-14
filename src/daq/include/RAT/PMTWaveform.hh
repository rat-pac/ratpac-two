#ifndef __RAT_PMTWaveform__
#define __RAT_PMTWaveform__

#include <RAT/PMTPulse.hh>
#include <TGraph.h>
#include <TObject.h>
#include <vector>

namespace RAT {

class PMTWaveform {
public:
  PMTWaveform();
  virtual ~PMTWaveform();
  virtual double GetHeight(double time);

  std::vector<PMTPulse *> fPulse;
};

} // namespace RAT

#endif
